/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAlsaSmartXConnector.cpp
 * @date   20 Sep. 2015
 * @brief  Connector that connects the Alsa callback functions with the
 * provided smartx ring buffers
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <limits>
#include <unistd.h>
#include "internal/audio/common/alsa_smartx_plugin/IasAlsaPluginIpc.hpp"
#include "internal/audio/common/alsa_smartx_plugin/IasAlsaHwConstraintsStatic.hpp"
#include "internal/audio/common/audiobuffer/IasAudioRingBuffer.hpp"
#include "internal/audio/common/IasAlsaTypeConversion.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"
#include "internal/audio/common/IasIntProcMutex.hpp"

/*
 * SmartX Plugin
 */
#include "alsa_smartx_plugin/IasAlsaSmartXConnector.hpp"

namespace IasAudio {

static const uint32_t cMaxFullNameLength = 256;
static const std::string cClassName = "IasAlsaSmartXConnector::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"
#define LOG_DEVICE "device=" + mFullName + ":"

IasAlsaSmartXConnector::IasAlsaSmartXConnector()
  :mLog(IasAudioLogging::registerDltContext("SXP", "SmartX Plugin"))
  ,mConnectionName()
  ,mFullName()
  ,mSmartxConnection(nullptr)
  ,mSetParams()
  ,mAlsaIoPlugData(nullptr)
  ,mNotificationDescriptor(-1)
  ,mAlsaCallbacks()
  ,mShmAreas(nullptr)
  ,mAlsaTransferAreas(nullptr)
  ,mTimeout(0)
  ,mHwPtr(0)
  ,mAvailMin(0)
  ,mRest(0)
  ,mFdSignal()
  ,mOpenOnceFd(-1)
{
  //Nothing to do here
}


IasAlsaSmartXConnector::~IasAlsaSmartXConnector()
{
  mFdSignal.close();

  if(mAlsaIoPlugData)
  {
    delete mAlsaIoPlugData;
  }
  if(mAlsaTransferAreas)
  {
    delete[] mAlsaTransferAreas;
  }
  if(mSmartxConnection)
  {
    delete mSmartxConnection;
  }
  closeOpenOnceFile();
}

int IasAlsaSmartXConnector::init(const char* name, const snd_pcm_stream_t& stream, const int& mode)
{
  // Limit the length of the name
  if (strlen(name) > cMaxFullNameLength)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Name of device too long (", strlen(name), "chars):", name);
    return -EINVAL;
  }
  // Init the Alsa Plugin Data
  int err = 0;
  mAlsaIoPlugData = new snd_pcm_ioplug_t();
  IAS_ASSERT(mAlsaIoPlugData != nullptr);

  mFullName.clear();
  mFullName.append(name);
  size_t pos = mFullName.find(":");
  if (pos != std::string::npos)
  {
    mFullName.replace(pos, 1, "_");
  }
  if (stream == SND_PCM_STREAM_PLAYBACK)
  {
    mFullName.append("_p");
  }
  else
  {
    mFullName.append("_c");
  }
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "Fully qualified connection name");

  mAlsaIoPlugData->version = SND_PCM_IOPLUG_VERSION;
  mAlsaIoPlugData->name = "SmartX IO Plugin";
  mAlsaIoPlugData->callback = &mAlsaCallbacks;
  mAlsaIoPlugData->private_data = this;
  mAlsaIoPlugData->poll_fd = -1;

  // Init the callback structure
  initCallbacks(stream);

  // Connect the shared Memory
  if((err = connectToSmartX()))
  {
    /**
     * @log A more detailed description of the error is provided by the method IasAlsaSmartXConnector::connectToSmartX.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Connect failed.");
    return err;
  }

  IasFdSignal::IasResult fdres = mFdSignal.open(mFullName, IasFdSignal::eIasRead);
  if (fdres != IasFdSignal::eIasOk)
  {
    // Logging already done inside IasFdSignal::open.
    return -EBADFD;
  }
  mAlsaIoPlugData->poll_events = POLLIN;
  mAlsaIoPlugData->poll_fd = mFdSignal.getFd();

  // Create the pcm device
  if((err = snd_pcm_ioplug_create(mAlsaIoPlugData, mConnectionName.c_str(), stream, mode)))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Fail to create the ioplug device.");
    return err;
  }

  if (mode & SND_PCM_NONBLOCK)
  {
    mAlsaIoPlugData->nonblock = true;
  }

  // Define HW Constraints.
  if((err = defineHwConstraints()))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Define Hw Constraints failed.");
    return err;
  }

  if (mAlsaIoPlugData->stream == SND_PCM_STREAM_PLAYBACK)
  {
    IAS_ASSERT(mSmartxConnection != nullptr); // already checked in connectToSmartX
    IasAudioRingBuffer* ringBuffer = mSmartxConnection->verifyAndGetRingBuffer();
    IAS_ASSERT(ringBuffer != nullptr);
    ringBuffer->resetFromWriter();
    if (mAlsaIoPlugData->nonblock == true)
    {
      // Avoid that the switchmatrix can retrieve PCM frames from the ring buffer,
      // until we go into state eIasRingBuffStreamingStateRunning.
      ringBuffer->setStreamingState(eIasRingBuffStreamingStateStopRead);
    }
  }

  return err;
}

int IasAlsaSmartXConnector::loadConfig(const snd_config_t* conf)
{
  snd_config_iterator_t i, nio;
  const char *pcmDeviceName;

  snd_config_for_each(i, nio, conf)
  {
    snd_config_t *n = snd_config_iterator_entry(i);
    const char *id;
    if (snd_config_get_id(n, &id) < 0)
      continue;
    if (strcmp(id, "comment") == 0 || strcmp(id, "type") == 0 || strcmp(id, "hint") == 0)
      continue;
    if (strcmp(id, "name") == 0) {
      if (snd_config_get_string(n, &pcmDeviceName) == 0)
      {
        mConnectionName = pcmDeviceName;
      }
      continue;
    }
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Unknown field:", id);
    return -EINVAL;
  }
  return 0;
}

int IasAlsaSmartXConnector::prepareStreamConnection()
{
  // Will search for the ringbuffer, in case it was changed
  // Performance reason
  (void)mSmartxConnection->verifyAndGetRingBuffer();

  return 0;
}

int IasAlsaSmartXConnector::getPathDelay(snd_pcm_sframes_t* frames)
{
  IasAudioIpcPluginInt32Data delayResponse;
  IasAudioCommonResult result;

  // Request the latency from the smart x
  if(eIasResultOk != (result = mSmartxConnection->getOutIpc()->push<IasAudioIpcPluginControl>(eIasAudioIpcGetLatency)))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "No IPC Packages can be sent, Result:", toString(result));
    return -EIO;
  }
  // Receive answer
  for(;;)
  {
    if(eIasResultOk != (result = mSmartxConnection->getInIpc()->pop<IasAudioIpcPluginInt32Data>(&delayResponse)))
    {
      // Errorcase Invalid Package
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error during IPC receive:", toString(result));
      break;
    }
    else
    {
      if(delayResponse.control == eIasAudioIpcGetLatency)
      {
        *frames = delayResponse.response;
      }
      else
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Wrong response.");
      }

      return 0;
    }

  }
  return -EIO;
}

int IasAlsaSmartXConnector::setHwParams(snd_pcm_hw_params_t* params)
{
  (void) params;
  IasAudioCommonResult result;
  IasAudioIpcPluginControlResponse response;

  mSetParams.numChannels = convertChannelCountAlsaToIas(mAlsaIoPlugData->channels);
  if(mSetParams.numChannels == 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Requested channel count is not supported.");
    return -EINVAL;
  }
  if(mAlsaTransferAreas)
  {
    delete mAlsaTransferAreas;
  }

  mAlsaTransferAreas = new snd_pcm_channel_area_t[mSetParams.numChannels];
  if(!mAlsaTransferAreas)
  {
    return -ENOMEM;
  }

  mSetParams.sampleRate = mAlsaIoPlugData->rate;
  if(0 == mSetParams.sampleRate)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Requested sample rate is not supported.");
    return -EINVAL;
  }

  mSetParams.dataFormat = convertFormatAlsaToIas(mAlsaIoPlugData->format);
  if(mSetParams.dataFormat == eIasFormatUndef)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Requested sample format is not supported.");
    return -EINVAL;
  }

  if(mAlsaIoPlugData->period_size > static_cast<int64_t>(std::numeric_limits<uint32_t>::max()) ||
     mAlsaIoPlugData->buffer_size > static_cast<int64_t>(std::numeric_limits<uint32_t>::max()))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Parameter error: period_size or buffer_size exceeds the numeric max of int32 is not supported.");
    return -EINVAL;
  }

  mSetParams.periodSize = static_cast<int32_t>(mAlsaIoPlugData->period_size);
  if(mSetParams.periodSize == 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Period size was zero.");
    return -EINVAL;
  }

  // Calculate the Period count.
  float periodCount = static_cast<float>(mAlsaIoPlugData->buffer_size) /
                             static_cast<float>(mAlsaIoPlugData->period_size);

  mSetParams.numPeriods = static_cast<int32_t>(periodCount + 0.1); // Assure the right Value (float to int cast)

  if(0.01 < (periodCount - static_cast<float>(mSetParams.numPeriods)))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Period is not a round value. buffer_size/period_size =", periodCount);
    return -EINVAL;
  }
  // Calculate timeout value for blocked wait on ring-buffer
  if (mSetParams.sampleRate != 0)
  {
    // Timeout will be equal to ring-buffer size
    mTimeout = mSetParams.numPeriods * mSetParams.periodSize / (mSetParams.sampleRate / 1000);
  }
  else
  {
    // Default timeout if samplerate is not set
    mTimeout = 500;
  }

  if(eIasResultOk != (result = mSmartxConnection->getOutIpc()->
     push<IasAudioIpcPluginParamData>(IasAudioIpcPluginParamData(eIasAudioIpcParameters,mSetParams))))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "No IPC Packages can be sent, Result:", toString(result));
    return -EIO;
  }
  for(;;)
  {
    if(eIasResultOk != (result = mSmartxConnection->getInIpc()->pop<IasAudioIpcPluginControlResponse>(&response)))
    {
      // Errorcase Invalid Package
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error during IPC receive:", toString(result));
      break;
    }
    else
    {
      if(response.control == eIasAudioIpcParameters)
      {
        if(response.response == eIasAudioIpcACK)
        {
          DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "Successfully set hw params");
          return 0;
        }
        else
        {
          DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "SmartXbar has not accepted the given parameters.");
          return -EINVAL;
        }
      }
      else
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Wrong response:", toString(response.control));
        break;
      }
    }
  }
  return -EIO;
}

int IasAlsaSmartXConnector::setSwParams(snd_pcm_sw_params_t *params)
{
  snd_pcm_sw_params_get_avail_min(params, &mAvailMin);

  // Inform the ring buffer about the avail_min value.
  IAS_ASSERT(mSmartxConnection != nullptr); // already checked in connectToSmartX
  IasAudioRingBuffer* ringBuffer = mSmartxConnection->verifyAndGetRingBuffer();
  IAS_ASSERT(ringBuffer != nullptr);
  ringBuffer->setAvailMin(static_cast<uint32_t>(mAvailMin));

  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "Set mAvailMin to ", static_cast<uint32_t>(mAvailMin));

  return 0;
}

int IasAlsaSmartXConnector::startStream()
{
  if (mAlsaIoPlugData->stream == SND_PCM_STREAM_PLAYBACK)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "Starting playback into alsa_smartx_plugin");
    IasAudioRingBuffer* ringBuffer = mSmartxConnection->verifyAndGetRingBuffer();
    IAS_ASSERT(ringBuffer != nullptr);

    // Allow that the switchmatrix can retrieve PCM frames from the ring buffer,
    ringBuffer->setStreamingState(eIasRingBuffStreamingStateRunning);
  }

  // Command & Control that the stream was started
  IasAudioCommonResult result;
  IasAudioIpcPluginControlResponse response;

  if(eIasResultOk != (result = mSmartxConnection->getOutIpc()->push<IasAudioIpcPluginControl>(eIasAudioIpcStart)))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "No IPC Packages can be sent, Result:", toString(result));
    return -EIO;
  }
  for(;;)
  {
    if(eIasResultOk != (result = mSmartxConnection->getInIpc()->pop<IasAudioIpcPluginControlResponse>(&response)))
    {
      // Errorcase Invalid Package
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error during IPC receive:", toString(result));
      break;
    }
    else
    {
      if(response.control == eIasAudioIpcStart)
      {
        if(response.response == eIasAudioIpcACK)
        {
          DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "Successfully started stream");
          return 0;
        }
        else
        {
          DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "SmartXbar has not accepted a stream start.");
          return -EINVAL;
        }
      }
      else
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Wrong response:", toString(response.control));
        break;
      }
    }
  }
  return -EIO;
}

int IasAlsaSmartXConnector::stopStream()
{
  // Command & Control that the stream was stopped
  IasAudioCommonResult result;
  IasAudioIpcPluginControlResponse response;

  if(eIasResultOk != (result = mSmartxConnection->getOutIpc()->push<IasAudioIpcPluginControl>(eIasAudioIpcStop)))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "No IPC Packages can be sent, Result:", toString(result));
    return -EIO;
  }
  for(;;)
  {
    if(eIasResultOk != (result = mSmartxConnection->getInIpc()->pop<IasAudioIpcPluginControlResponse>(&response)))
    {
      // Errorcase Invalid Package
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error during IPC receive:", toString(result));
      break;
    }
    else
    {
      if(response.control == eIasAudioIpcStop)
      {
        if(response.response == eIasAudioIpcACK)
        {
          DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "Successfully stopped stream");
          return 0;
        }
        else
        {
          DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "SmartXbar have not accepted a stream stop.");
          return -EINVAL;
        }
      }
      else
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Wrong response:", toString(response.control));
        break;
      }
    }
  }
  return -EIO;
}

int IasAlsaSmartXConnector::drainStream()
{
  if (mAlsaIoPlugData->stream == SND_PCM_STREAM_PLAYBACK)
  {
    IasAudioRingBuffer* ringBuffer = mSmartxConnection->verifyAndGetRingBuffer();
    if (ringBuffer != nullptr)
    {
      IasAudioRingBufferResult rbres;
      if (mRest > 0)
      {
        // Align the content of the ringbuffer to next full period size
        uint32_t offset = 0;
        uint32_t frames = mRest;
        rbres = ringBuffer->beginAccess(eIasRingBufferAccessWrite, &mShmAreas, &offset, &frames);
        if (rbres == eIasRingBuffOk)
        {
          DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "Number of rest frames to fill=", mRest, "Number of frames allowed to write=", frames);
          for(uint32_t i = 0; i < mSetParams.numChannels; ++i)
          {
            convertAreaIasToAlsa(mShmAreas[i], mAlsaTransferAreas + i);
          }
          snd_pcm_areas_silence(mAlsaTransferAreas, offset, mSetParams.numChannels, frames, convertFormatIasToAlsa(mSetParams.dataFormat));
        }
        rbres = ringBuffer->endAccess(eIasRingBufferAccessWrite, offset, frames);
        if (rbres != eIasRingBuffOk)
        {
          DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error during endAccess:", toString(rbres));
        }
      }
      uint32_t freeSpace = 0;
      rbres = ringBuffer->waitWrite(mSetParams.numPeriods, 1000);
      if (rbres == eIasRingBuffOk)
      {
        rbres = ringBuffer->updateAvailable(eIasRingBufferAccessWrite, &freeSpace);
        if (rbres == eIasRingBuffOk)
        {
          DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "Free space available after drain=", freeSpace);
        }
      }
      else if (rbres == eIasRingBuffTimeOut)
      {
        rbres = ringBuffer->updateAvailable(eIasRingBufferAccessWrite, &freeSpace);
        if (rbres == eIasRingBuffOk)
        {
          DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, LOG_DEVICE, "Timeout during waitWrite, free space=", freeSpace);
        }
      }
      else
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error during waitWrite:", toString(rbres));
      }
    }
  }

  return 0;
}

int IasAlsaSmartXConnector::handlePollREvents(struct pollfd *pfd,
                                              unsigned int nfds,
                                              unsigned short* revents)
{
  if(nfds != 1)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "The Amount of file descriptors is not equal 1. nfds=", nfds);
    return -EINVAL;
  }

  *revents = pfd[0].revents & ~(POLLIN | POLLOUT);
  if (pfd[0].revents & POLLIN)
  {
    unsigned short flag = (mAlsaIoPlugData->stream == SND_PCM_STREAM_PLAYBACK) ? POLLOUT : POLLIN;
    *revents |= flag;
  }
  return 0;
}

snd_pcm_sframes_t IasAlsaSmartXConnector::getFramePointer()
{

  snd_pcm_sframes_t returnValue = 0;

  if(mAlsaIoPlugData->state == SND_PCM_STATE_XRUN)
  {
    return -EPIPE;
  }

  IAS_ASSERT(mSmartxConnection != nullptr);
  IasAudioRingBuffer* ringBuffer = mSmartxConnection->verifyAndGetRingBuffer();
  IAS_ASSERT(ringBuffer != nullptr);
  uint32_t available = 0;
  if (mAlsaIoPlugData->stream == SND_PCM_STREAM_CAPTURE)
  {
    available = ringBuffer->getWriteOffset();
  }
  else
  {
    available = ringBuffer->getReadOffset();
  }
  returnValue = static_cast<snd_pcm_sframes_t>(available);

  return returnValue;
}

snd_pcm_sframes_t IasAlsaSmartXConnector::transferJob(const snd_pcm_channel_area_t *areas,
                                                      snd_pcm_uframes_t offset,
                                                      snd_pcm_uframes_t size, //< frames
                                                      const TransferDirection& direction)
{
  if (size == 0)
  {
    return size;
  }
  IasAudioRingBufferResult result = eIasRingBuffOk;
  uint32_t shmOffset = 0;
  if(size > static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Cannot handle the size, transfer is too long: size=", static_cast<int32_t>(size));
  }
  uint32_t shmFrames = static_cast<uint32_t>(size);

  IasRingBufferAccess shmAccess = (direction == eIasPlaybackTransfer)? eIasRingBufferAccessWrite : eIasRingBufferAccessRead;

  // Check if Ringbuffer is available.
  IasAudioRingBuffer* ringBuffer = mSmartxConnection->verifyAndGetRingBuffer();
  if(!ringBuffer)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Ringbuffer pointer is not valid, no ringbuffer is available.");
    return -EBADFD;
  }

  if (mAlsaIoPlugData->nonblock == false)
  {
    if (direction == eIasPlaybackTransfer)
    {
      result = ringBuffer->waitWrite(1, mTimeout);
    }
    else
    {
      result = ringBuffer->waitRead(1, mTimeout);
    }
    if (result == eIasRingBuffTimeOut)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, LOG_DEVICE, "Timeout while waiting for ring-buffer");
      return 0;
    }
    else if (result != eIasRingBuffOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error during wait:", toString(result));
      return 0;
    }
  }

  // Acquire lock
  if(eIasRingBuffOk !=
    (result = ringBuffer->beginAccess(shmAccess, &mShmAreas, &shmOffset, &shmFrames)))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Can't acquire buffer for access. IasAudioCommonResult:", toString(result));
    mAlsaIoPlugData->state = SND_PCM_STATE_XRUN;
    // Ensure we do a proper clean-up of the beginAccess call by calling endAccess. As we have an error during the beginAccess call
    // we should supply zero for parameters offset and frames, because else we maybe change the ringbuffer read or write pointer incorrectly.
    result = ringBuffer->endAccess(shmAccess, 0, 0);
    IAS_ASSERT(result == eIasRingBuffOk);
    (void)result;
    return -EPIPE;
  }

  if (mAlsaIoPlugData->nonblock == false)
  {
    // Blocking mode
    if (shmFrames == 0)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Xrun: No more space/samples in buffer");
      mAlsaIoPlugData->state = SND_PCM_STATE_XRUN;
      // Ensure we do a proper clean-up of the beginAccess call by calling endAccess
      result = ringBuffer->endAccess(shmAccess, shmOffset, shmFrames);
      IAS_ASSERT(result == eIasRingBuffOk);
      (void)result;
      return -EPIPE;
    }
  }
  else
  {
    // Nonblocking mode
    if (shmFrames == 0)
    {
      // Ensure we do a proper clean-up of the beginAccess call by calling endAccess
      result = ringBuffer->endAccess(shmAccess, shmOffset, 0);
      IAS_ASSERT(result == eIasRingBuffOk);
      (void)result;
      return -EAGAIN;
    }
  }

  // Convert areas
  if((mShmAreas[0].maxIndex+1) != mSetParams.numChannels)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Channelcount missmatch in current transferJob.");
    // Ensure we do a proper clean-up of the beginAccess call by calling endAccess
    result = ringBuffer->endAccess(shmAccess, shmOffset, shmFrames);
    IAS_ASSERT(result == eIasRingBuffOk);
    (void)result;
    return -EINVAL;
  }

  for(uint32_t i = 0; i < mSetParams.numChannels; ++i)
  {
    convertAreaIasToAlsa(mShmAreas[i], mAlsaTransferAreas + i);
  }

  /*
   * Copy Area Contents NOTE: A copy size over 2^32 will fail with no error.
   * The oversize is checked with the buffer size in hw params.
   */
  snd_pcm_format_t temporaryFormat = convertFormatIasToAlsa(mSetParams.dataFormat);
  uint32_t minFrameCount = std::min<uint32_t>(static_cast<uint32_t>(size), shmFrames);
  int err = 0;
  if (minFrameCount < mSetParams.periodSize)
  {
    // Remember the rest that is missing to get a full period size.
    // This is used in the drainStream method to align the ringbuffer fill level
    mRest = mSetParams.periodSize - minFrameCount;
  }

  if(direction == eIasPlaybackTransfer)
  {
    err = snd_pcm_areas_copy(mAlsaTransferAreas, shmOffset, areas, offset, mSetParams.numChannels, minFrameCount, temporaryFormat);
  }
  else
  {
    err = snd_pcm_areas_copy(areas, offset, mAlsaTransferAreas, shmOffset, mSetParams.numChannels, minFrameCount, temporaryFormat);
  }
  if(err)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error during copy the areas:", snd_strerror(err));
  }

  // Clear the FdSignal, because we've now really read one period size from the buffer
  mFdSignal.read();

  // Release lock
  if(eIasRingBuffOk !=
    (result = ringBuffer->endAccess(shmAccess, shmOffset, minFrameCount)))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Can't end the buffer access. IasAudioCommonResult:", toString(result));
    return -EPIPE;
  }
  // We check here for the error of the snd_pcm_areas_copy and return in this case with the error code, because
  // we always have to do the endAccess to release the lock
  if (err == 0)
  {
    return minFrameCount;
  }
  else
  {
    return err;
  }
}

snd_pcm_sframes_t IasAlsaSmartXConnector::getRealAvail()
{
  IasAudioRingBuffer* ringBuffer = mSmartxConnection->verifyAndGetRingBuffer();
  IAS_ASSERT(ringBuffer != nullptr);
  uint32_t available = 0;
  if (mAlsaIoPlugData->stream == SND_PCM_STREAM_CAPTURE)
  {
    ringBuffer->updateAvailable(eIasRingBufferAccessRead, &available);
  }
  else
  {
    ringBuffer->updateAvailable(eIasRingBufferAccessWrite, &available);
  }
  return static_cast<snd_pcm_sframes_t>(available);
}

  void IasAlsaSmartXConnector::closeOpenOnceFile()
  {
    if (mOpenOnceFd != -1)
    {
      int res = ftruncate(mOpenOnceFd, 0);
      if (res == 0)
      {
        res = lockf(mOpenOnceFd, F_ULOCK, 0);
      }
      else
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "device=" + mFullName + ":", "Cannot truncate open once file lock:", strerror(errno));
        res = -errno;
      }
      if (res == 0)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "device=" + mFullName + ":", "Successfully unlocked open once file lock");
      }
      else
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "device=" + mFullName + ":", "Cannot unlock open once file lock:", strerror(errno));
      }
      res = close(mOpenOnceFd);
      if (res == 0)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "device=" + mFullName + ":", "Successfully closed open once file lock");
        mOpenOnceFd = -1;
      }
      else
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "device=" + mFullName + ":", "Cannot close open once file lock:", strerror(errno));
      }
    }
  }

/*
 * ALSA: Callback Functions
 */
int IasAlsaSmartXConnector::snd_pcm_smartx_prepare(snd_pcm_ioplug_t *io)
{
  // Ensured by alsa framework
  IAS_ASSERT(io != nullptr);
  // Ensured by plugin init
  IAS_ASSERT(io->private_data != nullptr);
  return static_cast<IasAlsaSmartXConnector*>(io->private_data)->prepareStreamConnection();
}

int IasAlsaSmartXConnector::snd_pcm_smartx_start(snd_pcm_ioplug_t *io)
{
  // Ensured by alsa framework
  IAS_ASSERT(io != nullptr);
  // Ensured by plugin init
  IAS_ASSERT(io->private_data != nullptr);
  return static_cast<IasAlsaSmartXConnector*>(io->private_data)->startStream();
}

int IasAlsaSmartXConnector::snd_pcm_smartx_stop(snd_pcm_ioplug_t *io)
{
  // Ensured by alsa framework
  IAS_ASSERT(io != nullptr);
  // Ensured by plugin init
  IAS_ASSERT(io->private_data != nullptr);
  return static_cast<IasAlsaSmartXConnector*>(io->private_data)->stopStream();
}

int IasAlsaSmartXConnector::snd_pcm_smartx_drain(snd_pcm_ioplug_t *io)
{
  // Ensured by alsa framework
  IAS_ASSERT(io != nullptr);
  // Ensured by plugin init
  IAS_ASSERT(io->private_data != nullptr);
  return static_cast<IasAlsaSmartXConnector*>(io->private_data)->drainStream();
}

int IasAlsaSmartXConnector::snd_pcm_smartx_close(snd_pcm_ioplug_t *io)
{
  int res = 0;
  // Ensured by alsa framework
  IAS_ASSERT(io != nullptr);
  // Ensured by plugin init
  IAS_ASSERT(io->private_data != nullptr);
  // As we know the private_data to be IasAlsaSmartXConnector, it must have been initialised
  // in IasAlsaSmartXConnector::Init, which set io->private_data to `this`. Also io is a member
  // of IasAlsaSmartXConnector and thus is not valid anymore after deleting IasAlsaSmartXConnector
  // instance. Trying to set
  // io->private_data after this delete will cause use-after-free.
  delete static_cast<IasAlsaSmartXConnector*>(io->private_data);
  return res;
}

snd_pcm_sframes_t IasAlsaSmartXConnector::snd_pcm_smartx_pointer(snd_pcm_ioplug_t *io)
{
  // Ensured by alsa framework
  IAS_ASSERT(io != nullptr);
  // Ensured by plugin init
  IAS_ASSERT(io->private_data != nullptr);
  return static_cast<IasAlsaSmartXConnector*>(io->private_data)->getFramePointer();
}

snd_pcm_sframes_t IasAlsaSmartXConnector::snd_pcm_smartx_playback_transfer(snd_pcm_ioplug_t *io,
                                                 const snd_pcm_channel_area_t *areas,
                                                 snd_pcm_uframes_t offset,
                                                 snd_pcm_uframes_t size)
{
  // Ensured by alsa framework
  IAS_ASSERT(io != nullptr);
  IAS_ASSERT(areas != nullptr);
  // Ensured by plugin init
  IAS_ASSERT(io->private_data != nullptr);
  return static_cast<IasAlsaSmartXConnector*>(io->private_data)->transferJob(areas, offset, size, eIasPlaybackTransfer);
}

snd_pcm_sframes_t IasAlsaSmartXConnector::snd_pcm_smartx_capture_transfer(snd_pcm_ioplug_t *io,
                                                 const snd_pcm_channel_area_t *areas,
                                                 snd_pcm_uframes_t offset,
                                                 snd_pcm_uframes_t size)
{
  // Ensured by alsa framework
  IAS_ASSERT(io != nullptr);
  IAS_ASSERT(areas != nullptr);
  // Ensured by plugin init
  IAS_ASSERT(io->private_data != nullptr);
  return static_cast<IasAlsaSmartXConnector*>(io->private_data)->transferJob(areas, offset, size, eIasCaptureTransfer);
}

int IasAlsaSmartXConnector::snd_pcm_smartx_hw_params(snd_pcm_ioplug_t *io, snd_pcm_hw_params_t *params)
{
  // Ensured by alsa framework
  IAS_ASSERT(io != nullptr);
  IAS_ASSERT(params != nullptr);
  // Ensured by plugin init
  IAS_ASSERT(io->private_data != nullptr);
  return static_cast<IasAlsaSmartXConnector*>(io->private_data)->setHwParams(params);
}

int IasAlsaSmartXConnector::snd_pcm_smartx_sw_params(snd_pcm_ioplug_t *io, snd_pcm_sw_params_t *params)
{
  // Ensured by alsa framework
  IAS_ASSERT(io != nullptr);
  IAS_ASSERT(params != nullptr);
  // Ensured by plugin init
  IAS_ASSERT(io->private_data != nullptr);
  return static_cast<IasAlsaSmartXConnector*>(io->private_data)->setSwParams(params);
}

int IasAlsaSmartXConnector::snd_pcm_smartx_poll_revents(snd_pcm_ioplug_t *io, struct pollfd *pfd,
                                                        unsigned int nfds, unsigned short *revents )
{
  // Ensured by alsa framework
  IAS_ASSERT(io != nullptr);
  // Ensured by plugin init
  IAS_ASSERT(io->private_data != nullptr);
  if(revents != nullptr && pfd != nullptr)
  {
    return static_cast<IasAlsaSmartXConnector*>(io->private_data)->handlePollREvents( pfd, nfds, revents);
  }
  return -EBADFD;
}

int IasAlsaSmartXConnector::snd_pcm_smartx_delay(snd_pcm_ioplug_t *io, snd_pcm_sframes_t* frames)
{
  // Ensured by alsa framework
  IAS_ASSERT(io != nullptr);
  // Ensured by plugin init
  IAS_ASSERT(io->private_data != nullptr);
  if(frames != nullptr)
  {
    return static_cast<IasAlsaSmartXConnector*>(io->private_data)->getPathDelay(frames);
  }
  return -EBADFD;
}

snd_pcm_sframes_t IasAlsaSmartXConnector::snd_pcm_smartx_real_avail(snd_pcm_ioplug_t *io)
{
  // Ensured by alsa framework
  IAS_ASSERT(io != nullptr);
  // Ensured by plugin init
  IAS_ASSERT(io->private_data != nullptr);
  return static_cast<IasAlsaSmartXConnector*>(io->private_data)->getRealAvail();
}


/*
 * PRIVATE
 */

void IasAlsaSmartXConnector::initCallbacks(const snd_pcm_stream_t& stream)

{
  memset(&mAlsaCallbacks, 0, sizeof(mAlsaCallbacks));
  mAlsaCallbacks.start = snd_pcm_smartx_start;
  mAlsaCallbacks.stop = snd_pcm_smartx_stop;
  mAlsaCallbacks.drain = snd_pcm_smartx_drain;
  mAlsaCallbacks.pointer = snd_pcm_smartx_pointer;

  // Playback -> Write to smartx;
  mAlsaCallbacks.transfer = (stream == SND_PCM_STREAM_PLAYBACK) ?
      snd_pcm_smartx_playback_transfer : snd_pcm_smartx_capture_transfer;

  mAlsaCallbacks.close = snd_pcm_smartx_close;
  mAlsaCallbacks.hw_params = snd_pcm_smartx_hw_params;
  mAlsaCallbacks.sw_params = snd_pcm_smartx_sw_params;
  mAlsaCallbacks.prepare = snd_pcm_smartx_prepare;
  mAlsaCallbacks.delay = snd_pcm_smartx_delay;
  mAlsaCallbacks.poll_revents = snd_pcm_smartx_poll_revents;
  mAlsaCallbacks.get_real_avail = snd_pcm_smartx_real_avail;
}

#ifndef OPEN_ONCE_LOCK_PATH
#define OPEN_ONCE_LOCK_PATH "/run/smartx/"
#endif


int IasAlsaSmartXConnector::connectToSmartX()
{
  IasAudioCommonResult result(eIasResultOk);

  mSmartxConnection = new IasAlsaPluginShmConnection();
  IAS_ASSERT(mSmartxConnection != nullptr);

  // Establish the SmartX Connection
  if(eIasResultOk !=
     (result = mSmartxConnection->findConnection(mFullName)))
  {
    /**
     * @log <NAME> doesn't exist in shared memory.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Alsa plugin can't be connected to smartx. IasAudioCommonResult:", toString(result));
    return -ENODEV;
  }
  std::string openOnceFilename = OPEN_ONCE_LOCK_PATH + mFullName + ".lock";
  mOpenOnceFd = open(openOnceFilename.c_str(), O_CREAT | O_RDWR | O_SYNC, 0660);
  if (mOpenOnceFd < 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error creating or opening open once lock:", strerror(errno));
    return mOpenOnceFd;
  }
  int res = lockf(mOpenOnceFd, F_TLOCK, 0);
  if (res == 0)
  {
    pid_t myPid = getpid();
    pid_t pidInFile = 0;
    ssize_t bytes;
    bytes = read(mOpenOnceFd, &pidInFile, sizeof(pid_t));
    if (bytes < 0)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error while trying to read content of open once lock file:", strerror(errno));
      return -EINVAL;
    }
    if (myPid == pidInFile)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Alsa plugin for this device already opened by same process with pid=", myPid);
      return -EBUSY;
    }
    else
    {
      off_t offset = lseek(mOpenOnceFd, 0, SEEK_SET);
      if (offset < 0)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error adjusting offset for open once lock file:", strerror(errno));
        return -EBUSY;
      }
      bytes = write(mOpenOnceFd, &myPid, sizeof(pid_t));
      if (bytes >= 0)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "Successfully locked", openOnceFilename, "fd=", mOpenOnceFd, "pid=", myPid, "pidInFile=", pidInFile);
      }
      else
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error while trying to write content of open once lock file:", strerror(errno));
        return -EINVAL;
      }
    }
  }
  else
  {
    if (errno == EACCES || errno == EAGAIN)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Alsa plugin for this device already opened:", strerror(errno));
      return -EBUSY;
    }
    else
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error trying to lock the open once file:", strerror(errno));
      return -EINVAL;
    }
  }
  return 0;
}

int IasAlsaSmartXConnector::defineHwConstraints()
{
  // Get the Constraints from the shared memory
  if(!mSmartxConnection->getAlsaHwConstraints())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "No hardware constraints present in.");
    return -EBADFD;
  }

  IasAlsaHwConstraintsStatic constraints = *(mSmartxConnection->getAlsaHwConstraints());

  int err = 0;

  if(constraints.isValid)
  {
    // Set format constraints //
    if(!constraints.formats.list.size())
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "The IasAlsaHwConstraints format type list was not filled.");
      return -EINVAL;
    }
    unsigned int* formats = new unsigned int[constraints.formats.list.size()];
    IAS_ASSERT(formats != nullptr);

    auto formatIter = constraints.formats.list.begin();
    for(int i = 0; formatIter != constraints.formats.list.end(); ++formatIter, ++i)
    {
      formats[i] = (unsigned int)convertFormatIasToAlsa(*formatIter);
    }

    if(0 > (err = snd_pcm_ioplug_set_param_list(mAlsaIoPlugData,
                                                SND_PCM_IOPLUG_HW_FORMAT,
                                                (unsigned int)constraints.formats.list.size(),
                                                formats)))
    {
      delete[] formats;
      return err;
    }

    delete[] formats;

    // End format constraints. //

    // Set valid layouts. //
    if(!constraints.access.list.size())
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "The IasAlsaHwConstraints access type list was not filled.");
      return -EINVAL;
    }
    // We always allow rw and mmap access, that's why we have always two entries
    uint32_t accessListSize = static_cast<uint32_t>(constraints.access.list.size())*2;
    unsigned int* access_list = new unsigned int[accessListSize];
    IAS_ASSERT(access_list != nullptr);

    auto accessIter = constraints.access.list.begin();
    for(int i = 0; accessIter != constraints.access.list.end(); ++accessIter, i+=2)
    {
      access_list[i] = convertAccessTypeIasToAlsa(*accessIter, eIasAccessRw);
      access_list[i+1] = convertAccessTypeIasToAlsa(*accessIter, eIasAccessMmap);
    }

    if(0 > (err = snd_pcm_ioplug_set_param_list(mAlsaIoPlugData,
                                                SND_PCM_IOPLUG_HW_ACCESS,
                                                accessListSize,
                                                access_list)))
    {
      delete[] access_list;
      return err;
    }
    delete[] access_list;

    // End valid layouts //

    // Set Channel constraints. //
    if(constraints.channels.list.size())
    {
      unsigned int* channels = new unsigned int[constraints.channels.list.size()];
      IAS_ASSERT(channels != nullptr);

      auto channelIter = constraints.channels.list.begin();
      for(int i = 0; channelIter != constraints.channels.list.end(); ++channelIter, ++i)
      {
        channels[i] = convertChannelCountIasToAlsa(*channelIter);
      }

      if(0 > (err = snd_pcm_ioplug_set_param_list(mAlsaIoPlugData,
                                                  SND_PCM_IOPLUG_HW_CHANNELS,
                                                  (unsigned int)constraints.channels.list.size(),
                                                  channels)))
      {
        delete[] channels;
        return err;
      }
      delete[] channels;

    }
    else
    {
      // Set max/min values
      if(0 > (err = snd_pcm_ioplug_set_param_minmax(mAlsaIoPlugData,
                                                    SND_PCM_IOPLUG_HW_CHANNELS,
                                                    convertChannelCountIasToAlsa(constraints.channels.min),
                                                    convertChannelCountIasToAlsa(constraints.channels.max))))
      {
        return err;
      }

    }
    // End Channel constraints. //

    // Set Sample Rate constraints. //
    if(constraints.rate.list.size())
    {
      unsigned int* rate = new unsigned int[constraints.rate.list.size()];
      IAS_ASSERT(rate != nullptr);

      auto rateIter = constraints.rate.list.begin();
      for(int i = 0; rateIter != constraints.rate.list.end(); ++rateIter, ++i)
      {
        rate[i] = *rateIter;
      }

      if(0 > (err = snd_pcm_ioplug_set_param_list(mAlsaIoPlugData,
                                                  SND_PCM_IOPLUG_HW_RATE,
                                                  (unsigned int)constraints.rate.list.size(),
                                                  rate)))
      {
        delete[] rate;
        return err;
      }
      delete[] rate;

    }
    else
    {
      // Set max/min values
      if(0 > (err = snd_pcm_ioplug_set_param_minmax(mAlsaIoPlugData,
                                                    SND_PCM_IOPLUG_HW_RATE,
                                                    constraints.rate.min,
                                                    constraints.rate.max)))
      {
        return err;
      }

    }
    // End Sample Rate constraints. //


    // the actual period size
    // Set Periode size constraints. //
    if(constraints.period_size.list.size())
    {
      unsigned int* period_size = new unsigned int[constraints.period_size.list.size()];
      IAS_ASSERT(period_size != nullptr);

      auto periodSizeIter = constraints.period_size.list.begin();
      for(int i = 0; periodSizeIter != constraints.period_size.list.end(); ++periodSizeIter, ++i)
      {
        period_size[i] = *periodSizeIter;
      }

      if(0 > (err = snd_pcm_ioplug_set_param_list(mAlsaIoPlugData,
                                                  SND_PCM_IOPLUG_HW_PERIOD_BYTES,
                                                  (unsigned int)constraints.period_size.list.size(),
                                                  period_size)))
      {
        delete[] period_size;
        return err;
      }
      delete[] period_size;

    }
    else
    {
      // Set max/min values
      if(0 > (err = snd_pcm_ioplug_set_param_minmax(mAlsaIoPlugData,
                                                    SND_PCM_IOPLUG_HW_PERIOD_BYTES,
                                                    constraints.period_size.min,
                                                    constraints.period_size.max)))
      {
        return err;
      }

    }
    // Set Periode size constraints. //

    // Set Periode count constraints. //
    if(constraints.period_count.list.size())
    {
      unsigned int* period_count = new unsigned int[constraints.period_count.list.size()];
      IAS_ASSERT(period_count != nullptr);

      auto periodCountIter = constraints.period_count.list.begin();
      for(int i = 0; periodCountIter != constraints.period_count.list.end(); ++periodCountIter, ++i)
      {
        period_count[i] = *periodCountIter;
      }

      if(0 > (err = snd_pcm_ioplug_set_param_list(mAlsaIoPlugData,
                                                  SND_PCM_IOPLUG_HW_PERIODS,
                                                  (unsigned int)constraints.period_count.list.size(),
                                                  period_count)))
      {
        delete[] period_count;
        return err;
      }
      delete[] period_count;

    }
    else
    {
      // Set max/min values
      if(0 > (err = snd_pcm_ioplug_set_param_minmax(mAlsaIoPlugData,
                                                    SND_PCM_IOPLUG_HW_PERIODS,
                                                    constraints.period_count.min,
                                                    constraints.period_count.max)))
      {
        return err;
      }

    }
    // End Periode count constraints. //
    return 0;
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Hardware constrains are not initialized.");
    return -EAGAIN;
  }

  return -EINVAL;
}

}
