/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file IasAudioRingBufferMirror.cpp
 */

#include <chrono>
#include <iostream>
#include <alsa/asoundlib.h>
#include "internal/audio/common/audiobuffer/IasAudioRingBufferMirror.hpp"
#include "internal/audio/common/IasAlsaTypeConversion.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"


namespace IasAudio {

static const std::string cClassName = "IasAudioRingBufferMirror::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"
#define LOG_DEVICE "device=" + std::string(snd_pcm_name(mDevice)) + ":"

IasAudioRingBufferMirror::IasAudioRingBufferMirror()
:mDevice(nullptr)
,mNumChannels(0)
,mInitialized(false)
,mDeviceIsSet(false)
,mAccessInProgress(false)
,mFirstLoop(true)
,mPeriodSize(0)
,mTimeout_ms(-1)
,mNonBlockMode(false)
,mNumTransmittedFrames(0)
,mAudioTimestamp()
,mLog(IasAudioLogging::registerDltContext("RBM", "Audio Ringbuffer Mirror"))
,mTimeOutCnt(0)
{
  //Nothing to do here
}

IasAudioRingBufferMirror::~IasAudioRingBufferMirror()
{
  //Nothing to do here
}

IasAudioRingBufferResult IasAudioRingBufferMirror::init(uint32_t numChannels)
{
  if (numChannels == 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "numChannels may not be zero");
    return eIasRingBuffInvalidParam;
  }

  mNumChannels = numChannels;
  mInitialized = true;
  return eIasRingBuffOk;
}

IasAudioRingBufferResult IasAudioRingBufferMirror::setDeviceHandle(void* handle, uint32_t periodSize, uint32_t timeout_ms)
{
  if (handle != nullptr)
  {
    mDevice = static_cast<snd_pcm_t*>(handle);
    mDeviceIsSet = true;
    mPeriodSize = periodSize;
    mTimeout_ms = timeout_ms;
    mFirstLoop = true;
    return eIasRingBuffOk;
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "handle == nullptr");
    return eIasRingBuffInvalidParam;
  }
}

void IasAudioRingBufferMirror::setNonBlockMode(bool isNonBlocking)
{
  mNonBlockMode = isNonBlocking;
  std::string mode;
  if (isNonBlocking == true)
  {
    mode = "non-blocking";
  }
  else
  {
    mode = "blocking";
  }
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Set mode to", mode);
}

IasAudioRingBufferResult IasAudioRingBufferMirror::updateAvailable(IasRingBufferAccess access, uint32_t* samples)
{

  (void) access; //not needed, the alsa device will take care
  if (mInitialized == false || mDeviceIsSet == false)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Either ringbuffer is not initialized or ALSA device isn't set");
    return eIasRingBuffNotInitialized;
  }
  if (samples == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "samples == nullptr");
    return eIasRingBuffInvalidParam;
  }

  // The following functions are based on the direct_loop transfer method
  // of the example "pcm.c" provided in the "test" directory of the alsa-lib.
  do
  {
    snd_pcm_state_t state = snd_pcm_state(mDevice);
    if (state == SND_PCM_STATE_XRUN)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "is in state xrun, trying to recover");
      int err = xrunRecovery(-EPIPE);
      if (err < 0)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "failed to recover from xrun state");
        return eIasRingBuffAlsaXrunError; // XRUN recovery failed
      }
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "successfully recovered from xrun state");
      mFirstLoop = true;
    }
    else if (state == SND_PCM_STATE_SUSPENDED)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "is in state suspended, trying to recover");
      int err = xrunRecovery(-ESTRPIPE);
      if (err < 0)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "failed to recover from suspended state");
        return eIasRingBuffAlsaSuspendError; // SUSPEND recovery failed
      }
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "successfully recovered from suspended state");
    }

    int32_t avail = static_cast<int32_t>(snd_pcm_avail_update(mDevice));
    if (avail >= 0)
    {
      *samples = static_cast<uint32_t>(avail);
    }
    else
    {
      *samples = 0;
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "is unable to perform avail_update, trying to recover");
      int err = xrunRecovery(avail);
      if (err < 0)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "failed to recover from avail_update error");
        return eIasRingBuffAlsaError;
      }
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "successfully recovered from avail_update error");
      mFirstLoop = true;
      continue;
    }

    // If the mirror buffer is almost full (i.e., if the number of free samples
    // becomes smaller than one period),
    // -> start the playback, if this is the first period, or otherwise,
    // -> call snd_pcm_wait in order to wait till the playback device can consume the PCM frames.
    if (*samples < mPeriodSize)
    {
      if (mFirstLoop)
      {
        mFirstLoop = false;
        snd_pcm_state_t state = snd_pcm_state(mDevice);
        if (state != SND_PCM_STATE_RUNNING)
        {
          DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "ALSA device state:", snd_pcm_state_name(state));

          int alsaErrorCode = snd_pcm_start(mDevice);
          if (alsaErrorCode < 0)
          {
            DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "failed to start:", snd_strerror(alsaErrorCode));
            return eIasRingBuffAlsaError;
          }
          DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "successfully started");
        }
        else
        {
          DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "already running");
        }
      }
      else
      {
        // Exit the loop immediately, if we are in non-blocking mode.
        if (mNonBlockMode)
        {
          break;
        }
        // Wait until the playback buffer provides space for at least one period.
        int alsaErrorCode = snd_pcm_wait(mDevice, mTimeout_ms);
        if (alsaErrorCode == 0)
        {
          // Timeout handling.
          if(mTimeOutCnt%50 == 0)
          {
            DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "timed out after", mTimeout_ms, "ms, timeOutCnt:",mTimeOutCnt);
            DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE,"ALSA device state:", snd_pcm_state_name(state));
            DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE,"Samples/free space available in device:",*samples);
          }
          mTimeOutCnt++;
          return eIasRingBuffTimeOut;
        }
        if (alsaErrorCode < 0)
        {
          DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "is unable to perform wait:", snd_strerror(alsaErrorCode), "trying to recover");
          alsaErrorCode = xrunRecovery(alsaErrorCode);
          if (alsaErrorCode < 0)
          {
            DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "failed to recover from wait error");
            return eIasRingBuffAlsaError;
          }
          mFirstLoop = true;
          DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "successfully recovered from wait error");
        }
      }
      continue;
    }
  } while (*samples < mPeriodSize);
  // Stay within the loop as long as the playback buffer provides space for less than one period.


  // Get the current time stamp.
  auto durationSinceEpoch = std::chrono::high_resolution_clock::now().time_since_epoch();
  uint64_t timestamp   = std::chrono::duration_cast<std::chrono::microseconds>(durationSinceEpoch).count();

  mAudioTimestamp.timestamp = timestamp;
  mAudioTimestamp.numTransmittedFrames = mNumTransmittedFrames;

  return  eIasRingBuffOk;
}

IasAudioRingBufferResult IasAudioRingBufferMirror::beginAccess(IasAudioArea* area, uint32_t* offset, uint32_t* frames)
{
  if (mInitialized == false || mDeviceIsSet == false)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Not initialized");
    return eIasRingBuffNotInitialized;
  }
  if (area == NULL || offset == NULL || frames == NULL)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Invalid param");
    return eIasRingBuffInvalidParam;
  }
  if (mAccessInProgress)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Access already in progress");
    return eIasRingBuffNotAllowed;
  }
  const snd_pcm_channel_area_t* alsaArea;
  snd_pcm_uframes_t alsaFrames = static_cast<snd_pcm_uframes_t> (*frames);
  snd_pcm_uframes_t alsaOffset = static_cast<snd_pcm_uframes_t> (*offset);

  int32_t alsaErrorCode = static_cast<int32_t>(snd_pcm_mmap_begin(mDevice,&alsaArea,&alsaOffset,&alsaFrames));
  if (alsaErrorCode)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "is unable to perform mmap_begin, trying to recover");
    alsaErrorCode = xrunRecovery(alsaErrorCode);
    if (alsaErrorCode < 0)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "failed to recover from mmap_begin error");
      return eIasRingBuffAlsaError;
    }
    mFirstLoop = true;
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "successfully recovered from mmap_begin error");
  }

  mAccessInProgress.exchange(true);

  for (uint32_t i=0; i<mNumChannels; i++)
  {
    area[i].start = alsaArea[i].addr;
    area[i].first = alsaArea[i].first;
    area[i].step = alsaArea[i].step;
    area[i].index = i;
    area[i].maxIndex = mNumChannels-1;
  }
  *offset = static_cast<uint32_t>(alsaOffset);
  *frames = static_cast<uint32_t>(alsaFrames);

  return eIasRingBuffOk;
}

IasAudioRingBufferResult IasAudioRingBufferMirror::endAccess(uint32_t offset, uint32_t frames)
{

  snd_pcm_uframes_t alsaOffset = static_cast<snd_pcm_uframes_t>(offset);
  snd_pcm_uframes_t alsaFrames = static_cast<snd_pcm_uframes_t>(frames);
  if (mAccessInProgress)
  {
    mAccessInProgress.exchange(false);
    mNumTransmittedFrames += frames;
    int32_t alsaErrorCode = static_cast<int32_t>(snd_pcm_mmap_commit(mDevice,alsaOffset,alsaFrames));
    if (alsaErrorCode != static_cast<int32_t>(frames))
    {
      // Set the error code to -EPIPE. This is adopted from alsa-lib/test/pcm.c,
      // function direct_loop, error handling for (commitres != frames).
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "is unable to perform mmap_commit, error=", snd_strerror(alsaErrorCode));
      alsaErrorCode = -EPIPE;
    }

    if ((alsaErrorCode < 0) || (alsaErrorCode == -EPIPE))
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "is unable to perform mmap_commit, trying to recover");
      alsaErrorCode = xrunRecovery(alsaErrorCode);
      if (alsaErrorCode < 0)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "failed to recover from mmap_commit error");
        return eIasRingBuffAlsaError;
      }
      mFirstLoop = true;
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "successfully recovered from mmap_commit error");
    }
    return eIasRingBuffOk;
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "No access in progress, missing begin_access");
    return eIasRingBuffNotAllowed;
  }
}


IasAudioRingBufferResult IasAudioRingBufferMirror::getDataFormat(IasAudioCommonDataFormat *dataFormat) const
{
  snd_pcm_hw_params_t *hwParams;
  snd_pcm_format_t     format;

  if (dataFormat == NULL)
  {
    return eIasRingBuffInvalidParam;
  }

  if (mDevice == NULL)
  {
    return eIasRingBuffNotInitialized;
  }

  // Ask the ALSA device about its hardware parameters.
  snd_pcm_hw_params_alloca(&hwParams);
  int status = snd_pcm_hw_params_current(mDevice, hwParams);
  if (status)
  {
    return eIasRingBuffAlsaError;
  }

  // Get the format from the ALSA parameter struct.
  status = snd_pcm_hw_params_get_format (hwParams, &format);
  if (status < 0)
  {
    return eIasRingBuffAlsaError;
  }

  // Convert to IAS data type.
  *dataFormat = convertFormatAlsaToIas(format);

  return eIasRingBuffOk;
}

IasAudioRingBufferResult IasAudioRingBufferMirror::getTimestamp(IasAudioTimestamp *audioTimestamp) const
{
  if (audioTimestamp == nullptr)
  {
    return eIasRingBuffInvalidParam;
  }

  *audioTimestamp = mAudioTimestamp;
  return eIasRingBuffOk;
}

void IasAudioRingBufferMirror::startDevice() const
{
  int res = snd_pcm_start(mDevice);
  if (res < 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "failed to start ALSA device");
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "Successfully started ALSA device");
  }
}

/*
 * Private function: Underrun and suspend recovery
 */
int IasAudioRingBufferMirror::xrunRecovery(int err)
{
  const int cUnrecoverableError = -1;
  if (err == -EPIPE)
  {
    // under-run
    return snd_pcm_prepare(mDevice);
  }
  else if (err == -ESTRPIPE)
  {
    while ((err = snd_pcm_resume(mDevice)) == -EAGAIN)
    {
      sleep(1);   /* wait until the suspend flag is released */
    }
    if (err < 0)
    {
      err = snd_pcm_prepare(mDevice);
      if (err < 0)
      {
        return cUnrecoverableError;
      }
    }
    return 0;
  }
  return err;
}


} // namespace Ias
