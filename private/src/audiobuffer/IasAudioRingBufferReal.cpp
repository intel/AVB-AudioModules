/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAudioRingBufferReal.cpp
 * @date   2015
 * @brief
 */

#include "internal/audio/common/audiobuffer/IasAudioRingBufferReal.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"
#include "internal/audio/common/IasFdSignal.hpp"

#include <limits.h>
#include <chrono>
#include <stdio.h>
#include <stdlib.h>
#include <asm/errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

namespace IasAudio
{

using namespace boost::interprocess;


IasAudioRingBufferReal::IasAudioRingBufferReal()
  :mPeriodSize(0)
  ,mNumPeriods(0)
  ,mNumChannels(0)
  ,mNumChannelsMax(0)
  ,mReadOffset(0)
  ,mWriteOffset(0)
  ,mDataFormat(eIasFormatFloat32)
  ,mSampleSize(0)
  ,mBufferLevel(0)
  ,mShared(false)
  ,mInitialized(false)
  ,mReadInProgress(false)
  ,mWriteInProgress(false)
  ,mDataBuf(nullptr)
  ,mMutex()
  ,mMutexReadInProgress()
  ,mMutexWriteInProgress()
  ,mCondRead()
  ,mCondWrite()
  ,mReadWaitLevel(0)
  ,mWriteWaitLevel(0)
  ,mAudioTimestampAccessRead()
  ,mAudioTimestampAccessWrite()
  ,mStreamingState(eIasRingBuffStreamingStateRunning)
  ,mFdSignal(nullptr)
  ,mDeviceType(eIasDeviceTypeUndef)
  ,mAvailMin(0u)
  ,mHwPtrRead(0)
  ,mHwPtrWrite(0)
  ,mBoundary(0)
{
  //Nothing to do here
}

IasAudioRingBufferReal::~IasAudioRingBufferReal()
{
  //Nothing to do here
}

IasAudioRingBufferResult IasAudioRingBufferReal::init(uint32_t periodSize,
                                                      uint32_t nPeriods,
                                                      uint32_t nChannels,
                                                      IasAudioCommonDataFormat dataFormat,
                                                      void* dataBuf,
                                                      bool shared,
                                                      IasMetaData* metaData)
{

  (void)metaData;
  mShared = shared;
  if (periodSize == 0 ||
      nChannels == 0 ||
      dataBuf == 0 ||
      nPeriods == 0 ||
      metaData == nullptr)
  {
    return eIasRingBuffInvalidParam;
  }
  mSampleSize = toSize(dataFormat);
  if (mSampleSize == -1)
  {
    return eIasRingBuffInvalidSampleSize;
  }
  mPeriodSize = periodSize;
  mNumPeriods = nPeriods;
  mNumChannels = nChannels;
  mDataFormat = dataFormat;
  mDataBuf = dataBuf;
  // Set it to an initial value similar like the default value used in the alsa-lib.
  mBoundary = mPeriodSize*mNumPeriods;
  while (mBoundary * 2 <= static_cast<uint64_t>(LONG_MAX) - mPeriodSize*mNumPeriods)
  {
    mBoundary *= 2;
  }
  mInitialized = true;

  return eIasRingBuffOk;
}

IasAudioRingBufferResult IasAudioRingBufferReal::updateAvailable(IasRingBufferAccess access, uint32_t *samples)
{
  if (samples == NULL || access == eIasRingBufferAccessUndef)
  {
    return eIasRingBuffInvalidParam;
  }
  if (mInitialized == false)
  {
    return eIasRingBuffNotInitialized;
  }

  if(access == eIasRingBufferAccessRead)
  {
    *samples = mBufferLevel;
  }
  else
  {
    *samples = mNumPeriods*mPeriodSize - mBufferLevel;
  }
  return  eIasRingBuffOk;
}

IasAudioRingBufferResult IasAudioRingBufferReal::beginAccess(IasRingBufferAccess access, uint32_t* offset, uint32_t* frames)
{

  if (offset == NULL || frames == NULL || access == eIasRingBufferAccessUndef)
  {
    return eIasRingBuffInvalidParam;
  }
  if (mInitialized == false)
  {
    return eIasRingBuffNotInitialized;
  }

  if (access == eIasRingBufferAccessRead)
  {
    if (mReadInProgress)
    {
      return eIasRingBuffNotAllowed;
    }
    mReadInProgress.exchange(true);
    mMutexReadInProgress.lock();
    *offset = mReadOffset;

    // If reading from the buffer is blocked -> return with 0 frames.
    if (mStreamingState == eIasRingBuffStreamingStateStopRead)
    {
      *frames = 0;
    }
    else
    {
      if ( (*frames) > mBufferLevel )
      {
        *frames = mBufferLevel;
      }
      if ( (mReadOffset + *frames) >= (mNumPeriods*mPeriodSize))
      {
        *frames = (mNumPeriods*mPeriodSize) - mReadOffset;
      }
    }
  }
  else
  {
    if (mWriteInProgress)
    {
      return eIasRingBuffNotAllowed;
    }
    mWriteInProgress.exchange(true);
    mMutexWriteInProgress.lock();
    *offset = mWriteOffset;

    // If writing into the buffer is blocked -> return with 0 frames.
    if (mStreamingState == eIasRingBuffStreamingStateStopWrite)
    {
      *frames = 0;
    }
    else
    {
      if ( (*frames) > (mNumPeriods*mPeriodSize - mBufferLevel) )
      {
        *frames = (mNumPeriods*mPeriodSize - mBufferLevel);
      }
      if ( (mWriteOffset + *frames) >= (mNumPeriods*mPeriodSize) )
      {
        *frames = (mNumPeriods*mPeriodSize) - mWriteOffset;
      }
    }
  }
  return eIasRingBuffOk;
}

IasAudioRingBufferResult IasAudioRingBufferReal::endAccess(IasRingBufferAccess access, uint32_t offset, uint32_t frames)
{
  (void)offset;
  if (access == eIasRingBufferAccessUndef)
  {
    return eIasRingBuffInvalidParam;
  }
  if (access == eIasRingBufferAccessRead )
  {
    if (mReadInProgress)
    {
      if ( static_cast<int32_t>(mBufferLevel - frames) < 0)
      {
        return eIasRingBuffInvalidParam;
      }
      IasLockGuard lock(&mMutex);
      if( (mReadOffset+frames) == (mNumPeriods*mPeriodSize) )
      {
        mReadOffset = 0;
      }
      else if ( (mReadOffset+frames) > (mNumPeriods*mPeriodSize) )
      {
        return eIasRingBuffInvalidParam;
      }
      else
      {
        mReadOffset += frames;
      }
      mBufferLevel -= frames;
      mHwPtrRead += frames;
      if (static_cast<uint64_t>(mHwPtrRead) >= mBoundary)
      {
        mHwPtrRead -= mBoundary;
      }

      // Get the current time stamp and write it to mAudioTimestampAccessRead
      auto durationSinceEpoch = std::chrono::high_resolution_clock::now().time_since_epoch();
      uint64_t timestamp   = std::chrono::duration_cast<std::chrono::microseconds>(durationSinceEpoch).count();
      mAudioTimestampAccessRead.timestamp = timestamp;
      mAudioTimestampAccessRead.numTransmittedFrames += frames;

      mReadInProgress.exchange(false);
      mMutexReadInProgress.unlock();
      if (mBufferLevel <= mWriteWaitLevel)
      {
        mCondWrite.signal();
      }
      if (mFdSignal != nullptr && frames > 0 && mDeviceType == eIasDeviceTypeSource)
      {
        // The snd_pcm_wait function on the plugin (client) side shall not return
        // before at least avail_min frames are available (free).
        if ((mNumPeriods*mPeriodSize - mBufferLevel) >= mAvailMin)
        {
          mFdSignal->write();
        }
      }
    }
  }
  else
  {
    if (mWriteInProgress)
    {
      if ( (mBufferLevel + frames) > (mNumPeriods*mPeriodSize) )
      {
        return eIasRingBuffInvalidParam;
      }
      IasLockGuard lock(&mMutex);
      if ( (mWriteOffset + frames) == (mNumPeriods*mPeriodSize) )
      {
        mWriteOffset = 0;
      }
      else if ( (mWriteOffset + frames) > (mNumPeriods*mPeriodSize) )
      {
        return eIasRingBuffInvalidParam;
      }
      else
      {
        mWriteOffset += frames;
      }
      mBufferLevel += frames;
      mHwPtrWrite += frames;
      if (static_cast<uint64_t>(mHwPtrWrite) >= mBoundary)
      {
        mHwPtrWrite -= mBoundary;
      }

      // Get the current time stamp and write it to mAudioTimestampAccessWrite
      auto durationSinceEpoch = std::chrono::high_resolution_clock::now().time_since_epoch();
      uint64_t timestamp   = std::chrono::duration_cast<std::chrono::microseconds>(durationSinceEpoch).count();
      mAudioTimestampAccessWrite.timestamp = timestamp;
      mAudioTimestampAccessWrite.numTransmittedFrames += frames;

      mWriteInProgress.exchange(false);
      mMutexWriteInProgress.unlock();
      if (mBufferLevel >= mReadWaitLevel)
      {
        mCondRead.signal();
      }
      if (mFdSignal != nullptr && frames > 0 && mDeviceType == eIasDeviceTypeSink)
      {
        // The snd_pcm_wait function on the plugin (client) side shall not return
        // before at least avail_min frames are available (filled).
        if (mBufferLevel >= mAvailMin)
        {
          mFdSignal->write();
        }
      }
    }
  }
  return eIasRingBuffOk;
}

void IasAudioRingBufferReal::triggerFdSignal()
{
  if (mFdSignal != nullptr)
  {
    mFdSignal->write();
  }
}

IasAudioRingBufferResult IasAudioRingBufferReal::getDataFormat(IasAudioCommonDataFormat *dataFormat) const
{
  if (dataFormat == nullptr)
  {
    return eIasRingBuffInvalidParam;
  }

  *dataFormat = mDataFormat;
  return eIasRingBuffOk;
}

IasAudioRingBufferResult IasAudioRingBufferReal::waitWrite(uint32_t numPeriods, uint32_t timeout_ms )
{
  if ( (numPeriods > mNumPeriods) || numPeriods == 0 || timeout_ms == 0)
  {
    return eIasRingBuffInvalidParam;
  }

  IasAudioRingBufferResult result = eIasRingBuffOk;
  IasLockGuard lock(&mMutex);
  mWriteWaitLevel = (mNumPeriods-numPeriods)*mPeriodSize;
  IasIntProcCondVar::IasResult cndres = IasIntProcCondVar::eIasOk;
  if (mBufferLevel > mWriteWaitLevel)
  {
    cndres = mCondWrite.wait(mMutex, timeout_ms);
    if (cndres == IasIntProcCondVar::eIasTimeout)
    {
      result = eIasRingBuffTimeOut;
    }
    else if (cndres != IasIntProcCondVar::eIasOk)
    {
      result = eIasRingBuffCondWaitFailed;
    }
  }
  return result;
}

IasAudioRingBufferResult IasAudioRingBufferReal::waitRead(uint32_t numPeriods, uint32_t timeout_ms)
{
  if ( (numPeriods > mNumPeriods) || numPeriods == 0 || timeout_ms == 0)
  {
    return eIasRingBuffInvalidParam;
  }

  IasAudioRingBufferResult result = eIasRingBuffOk;
  IasLockGuard lock(&mMutex);
  mReadWaitLevel = numPeriods*mPeriodSize;
  IasIntProcCondVar::IasResult cndres = IasIntProcCondVar::eIasOk;
  if (mBufferLevel < mReadWaitLevel)
  {
    cndres = mCondRead.wait(mMutex, timeout_ms);
    if (cndres == IasIntProcCondVar::eIasTimeout)
    {
      result = eIasRingBuffTimeOut;
    }
    else if (cndres != IasIntProcCondVar::eIasOk)
    {
      result = eIasRingBuffCondWaitFailed;
    }
  }
  return result;
 }


IasAudioRingBufferResult IasAudioRingBufferReal::getTimestamp(IasRingBufferAccess access, IasAudioTimestamp *audioTimestamp)
{
  if (audioTimestamp == nullptr)
  {
    return eIasRingBuffInvalidParam;
  }

  IasLockGuard lock(&mMutex);
  if (access == eIasRingBufferAccessRead)
  {
    *audioTimestamp = mAudioTimestampAccessRead;
  }
  else if (access == eIasRingBufferAccessWrite)
  {
    *audioTimestamp = mAudioTimestampAccessWrite;
  }
  else
  {
    return eIasRingBuffInvalidParam;
  }
  return eIasRingBuffOk;
}


void IasAudioRingBufferReal::resetFromWriter()
{
  mMutexReadInProgress.lock();
  mReadOffset  = 0;
  mWriteOffset = 0;
  mBufferLevel = 0;
  mMutexReadInProgress.unlock();
};


void IasAudioRingBufferReal::resetFromReader()
{
  mMutexWriteInProgress.lock();
  mReadOffset  = 0;
  mWriteOffset = 0;
  mBufferLevel = 0;
  mMutexWriteInProgress.unlock();
};

void IasAudioRingBufferReal::setAvailMin(uint32_t availMin)
{
  mAvailMin = availMin;
}

void IasAudioRingBufferReal::setBoundary(uint64_t boundary)
{
  mBoundary = boundary;
  // Additionally clear the ever increasing hw_ptr here to align them to the state of the hw_ptr
  // maintained in the ALSA pcm device structure.
  mHwPtrRead = 0;

  //Set this to mAvailMin since that will cause capture applications to start reading
  //immedietely and not prevent SmartX from filling the buffer. Otherwise, there is a deadlock
  //as SmartX waits for the application to read data before it writes more data and
  //the application waits for SmartX to write more data before it reads more data.
  //TODO: Capture devices need to know how much data is in the buffer and query mHwPtrWrite
  //when calling the ioplug pointer() callback. Instead, they should query some application pointer
  //and subtract if from the hardware pointer to make this determination, instead of holding
  //that value in one data member. Then, we can set the application pointer to 0 here
  //instead of holding the current availability in one "hardware" pointer (which in this case
  //really be something like (mHwPtrWrite - mApplPtrWrite)).
  mHwPtrWrite = mAvailMin;
}

void IasAudioRingBufferReal::setFdSignal(IasFdSignal *fdSignal, IasDeviceType deviceType)
{
  IAS_ASSERT(fdSignal != nullptr);
  mFdSignal = fdSignal;
  mDeviceType = deviceType;
}

uint32_t IasAudioRingBufferReal::getPeriodSize() const
{
  return mPeriodSize;
}

uint32_t IasAudioRingBufferReal::getNumberPeriods() const
{
  return mNumPeriods;
}

void IasAudioRingBufferReal::zeroOut()
{
  // Lock both mutexes, to ensure nobody is accessing the buffer right now
  mMutexReadInProgress.lock();
  mMutexWriteInProgress.lock();
  uint32_t sizeOfBufferInBytes = mNumPeriods*mPeriodSize*mNumChannels*mSampleSize;
  IAS_ASSERT(getDataBuffer() != nullptr);
  memset(getDataBuffer(), 0, sizeOfBufferInBytes);
  mMutexWriteInProgress.unlock();
  mMutexReadInProgress.unlock();
}


}
