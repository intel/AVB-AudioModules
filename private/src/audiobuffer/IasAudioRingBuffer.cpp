/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAudioRingBuffer.cpp
 */

#include "internal/audio/common/audiobuffer/IasAudioRingBuffer.hpp"
#include "internal/audio/common/audiobuffer/IasAudioRingBufferReal.hpp"
#include "internal/audio/common/audiobuffer/IasAudioRingBufferMirror.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"

#include "internal/audio/common/IasDataProbe.hpp"
#include <memory>

namespace IasAudio
{

IasAudioRingBuffer::IasAudioRingBuffer()
:mRingBufReal(nullptr)
,mRingBufMirror(nullptr)
,mAreas(nullptr)
,mReal(true)
,mNumChannels(0)
,mName("uninitialized")
{
  //Nothing to do here
}

IasAudioRingBuffer::~IasAudioRingBuffer()
{
  if(mAreas != nullptr)
  {
    delete[] mAreas;
  }
}

IasAudioRingBufferResult IasAudioRingBuffer::init(uint32_t periodSize,
                                                       uint32_t nPeriods,
                                                       uint32_t nChannels,
                                                       IasAudioCommonDataFormat dataFormat,
                                                       void* dataBuf,
                                                       bool shared,
                                                       IasMetaData* metaData,
                                                       IasAudioRingBufferReal* ringBufReal)
{
  if (ringBufReal == nullptr     ||
      dataBuf == nullptr         ||
      metaData == nullptr        ||
      nChannels == 0             ||
      nPeriods == 0              ||
      periodSize == 0    )
  {
    return eIasRingBuffInvalidParam;
  }
  mRingBufReal = ringBufReal;
  IasAudioRingBufferResult res = mRingBufReal->init(periodSize,
                                                    nPeriods,
                                                    nChannels,
                                                    dataFormat,
                                                    dataBuf,
                                                    shared,
                                                    metaData);
  if(res != eIasRingBuffOk)
  {
    return res;
  }

  mAreas = new IasAudioArea[nChannels];
  IAS_ASSERT(mAreas != nullptr);
  mNumChannels = nChannels;

  for(uint32_t i=0; i<nChannels; i++)
  {
    mAreas[i].start    = mRingBufReal->getDataBuffer();
    mAreas[i].index    = i;
    mAreas[i].maxIndex = nChannels-1;
    mAreas[i].step     = toSize(dataFormat) * 8;
    mAreas[i].first    = nPeriods * periodSize * toSize(dataFormat) * 8 * i;
  }

  mReal = true;

  return res;
}

IasAudioRingBufferResult IasAudioRingBuffer::init(uint32_t nChannels,
                                                  IasAudioRingBufferMirror* ringBufMirror)
{
  if (ringBufMirror == nullptr || nChannels == 0)
  {
    return eIasRingBuffInvalidParam;
  }
  mNumChannels = nChannels;
  mAreas = new IasAudioArea[nChannels];
  IAS_ASSERT(mAreas != nullptr);
  mRingBufMirror = ringBufMirror;
  IasAudioRingBufferResult res = mRingBufMirror->init(nChannels);
  IAS_ASSERT(res == eIasRingBuffOk);
  (void)res;
  mReal = false;

  return eIasRingBuffOk;
}

IasAudioRingBufferResult IasAudioRingBuffer::setup(IasAudioRingBufferReal* ringBufReal)
{

  if (ringBufReal == nullptr)
  {
    return eIasRingBuffInvalidParam;
  }
  mRingBufReal = ringBufReal;

  mNumChannels = mRingBufReal->getNumChannels();
  if(mNumChannels == 0)
  {
    return eIasRingBuffNotInitialized;
  }

  void* dataBuf = mRingBufReal->getDataBuffer();
  IAS_ASSERT(dataBuf != nullptr);

  uint32_t sampleSize = mRingBufReal->getSampleSize();
  IAS_ASSERT(sampleSize > 0);

  uint32_t periodSize = mRingBufReal->getPeriodSize();
  IAS_ASSERT(periodSize > 0);

  uint32_t numberPeriods = mRingBufReal->getNumberPeriods();
  IAS_ASSERT(numberPeriods > 0);

  mAreas = new IasAudioArea[mNumChannels];
  IAS_ASSERT(mAreas != nullptr);
  for(uint32_t i=0; i<mNumChannels; i++)
  {
    mAreas[i].start    = dataBuf;
    mAreas[i].index    = i;
    mAreas[i].maxIndex = mNumChannels-1;
    mAreas[i].step     = sampleSize * 8;
    mAreas[i].first    = numberPeriods * periodSize * sampleSize * 8 * i;
  }
  mReal = true;
  return eIasRingBuffOk;
}

IasAudioRingBufferResult IasAudioRingBuffer::setDeviceHandle(void* handle, uint32_t periodSize, uint32_t timeout_ms)
{
  if (handle == NULL)
  {
    return eIasRingBuffInvalidParam;
  }
  if (!mReal)
  {
    //if we are here, the init function must be called ( mReal = false)
    //but then, the mRingBuffMirror pointer was checked before in init function, so no check necessary here
    IasAudioRingBufferResult res = mRingBufMirror->setDeviceHandle(handle, periodSize, timeout_ms);
    return res;
  }
  else
  {
    return eIasRingBuffNotAllowed;
  }
}

void IasAudioRingBuffer::clearDeviceHandle()
{
  if (!mReal)
  {
    mRingBufMirror->clearDeviceHandle();
  }
}

IasAudioRingBufferResult IasAudioRingBuffer::setNonBlockMode(bool isNonBlocking)
{
  // This function makes sense only for ring buffers that are not of type real.
  // If the ring buffer is of type real, we return with error message.
  if (!mReal)
  {
    IAS_ASSERT(mRingBufMirror != nullptr);
    mRingBufMirror->setNonBlockMode(isNonBlocking);
    return eIasRingBuffOk;
  }
  else
  {
    return eIasRingBuffNotAllowed;
  }
}


void IasAudioRingBuffer::setAvailMin(uint32_t availMin)
{
  if (mReal == true)
  {
    mRingBufReal->setAvailMin(availMin);
  }
}


void IasAudioRingBuffer::setBoundary(uint64_t boundary)
{
  if (mReal == true)
  {
    mRingBufReal->setBoundary(boundary);
  }
}


void IasAudioRingBuffer::setFdSignal(IasFdSignal *fdSignal, IasDeviceType deviceType)
{
  if (mReal == true)
  {
    mRingBufReal->setFdSignal(fdSignal, deviceType);
  }
}


IasAudioRingBufferResult IasAudioRingBuffer::updateAvailable(IasRingBufferAccess access, uint32_t* samples)
{
  if (samples == NULL || access == eIasRingBufferAccessUndef)
  {
    return eIasRingBuffInvalidParam;
  }
  if (mReal)
  {
    return mRingBufReal->updateAvailable(access,samples);
  }
  else
  {
    return mRingBufMirror->updateAvailable(access,samples);
  }
}

IasAudioRingBufferResult IasAudioRingBuffer::beginAccess(IasRingBufferAccess access, IasAudioArea** area, uint32_t* offset, uint32_t* frames)
{
  IasAudioRingBufferResult res = eIasRingBuffOk;
  if (area == NULL || offset == NULL || frames == NULL  || access == eIasRingBufferAccessUndef)
  {
    return eIasRingBuffInvalidParam;
  }
  if (mReal)
  {
    *area = mAreas;
    res = mRingBufReal->beginAccess(access,offset,frames);
  }
  else
  {
    res = mRingBufMirror->beginAccess(mAreas,offset,frames);
    *area = mAreas;
  }
  return res;
}

IasAudioRingBufferResult IasAudioRingBuffer::endAccess(IasRingBufferAccess access, uint32_t offset, uint32_t frames)
{

  if (access == eIasRingBufferAccessUndef)
  {
    return eIasRingBuffInvalidParam;
  }
  if (mReal)
  {
    return mRingBufReal->endAccess(access,offset,frames);
  }
  else
  {
    return mRingBufMirror->endAccess(offset,frames);
  }
}


void IasAudioRingBuffer::triggerFdSignal()
{
  if (mReal)
  {
    mRingBufReal->triggerFdSignal();
  }
}


IasAudioRingBufferResult IasAudioRingBuffer::getAreas(IasAudioArea** areas) const
{
  if (areas == NULL)
  {
    return eIasRingBuffInvalidParam;
  }
  if (!mReal)
  {
    //not allowed for mirror buffer, because the areas are only accessible via direct alsa function and not managed by ringbuffer
    return eIasRingBuffNotAllowed;
  }
  *areas = mAreas;
  return eIasRingBuffOk;
}

IasAudioRingBufferResult IasAudioRingBuffer::getDataFormat(IasAudioCommonDataFormat *dataFormat) const
{
  if (dataFormat == NULL)
  {
    return eIasRingBuffInvalidParam;
  }
  if (mReal)
  {
    return mRingBufReal->getDataFormat(dataFormat);
  }
  else
  {
    return mRingBufMirror->getDataFormat(dataFormat);
  }
}

IasAudioRingBufferResult IasAudioRingBuffer::waitRead(uint32_t numPeriods, uint32_t timeout_ms)
{
  if (mReal)
  {
    return mRingBufReal->waitRead(numPeriods, timeout_ms);
  }
  else
  {
    return eIasRingBuffNotAllowed;
  }
}

IasAudioRingBufferResult IasAudioRingBuffer::waitWrite(uint32_t numPeriods, uint32_t timeout_ms)
{
  if (mReal)
  {
    return mRingBufReal->waitWrite(numPeriods, timeout_ms);
  }
  else
  {
    return eIasRingBuffNotAllowed;
  }
}

IasAudioRingBufferResult IasAudioRingBuffer::getTimestamp(IasRingBufferAccess access, IasAudioTimestamp *audioTimestamp) const
{
  IasAudioRingBufferResult result;
  if (mReal)
  {
    result = mRingBufReal->getTimestamp(access, audioTimestamp);
  }
  else
  {
    result = mRingBufMirror->getTimestamp(audioTimestamp);
  }
  return result;
}

IasAudioRingBufferResult IasAudioRingBuffer::setStreamingState(IasAudioRingBufferStreamingState streamingState)
{
  if (mReal)
  {
    mRingBufReal->setStreamingState(streamingState);
    return eIasRingBuffOk;
  }
  else
  {
    return eIasRingBuffNotAllowed;
  }
}

IasAudioRingBufferStreamingState IasAudioRingBuffer::getStreamingState() const
{
  if (mReal)
  {
    return mRingBufReal->getStreamingState();
  }
  else
  {
    return eIasRingBuffStreamingStateUndefined;
  }
}

uint32_t IasAudioRingBuffer::getReadOffset() const
{
  if (mReal)
  {
    return mRingBufReal->getReadOffset();
  }
  else
  {
    return 0;
  }
}

uint32_t IasAudioRingBuffer::getWriteOffset() const
{
  if (mReal)
  {
    return mRingBufReal->getWriteOffset();
  }
  else
  {
    return 0;
  }
}

int64_t IasAudioRingBuffer::getHwPtrRead() const
{
  if (mReal)
  {
    return mRingBufReal->getHwPtrRead();
  }
  else
  {
    return 0;
  }
}

int64_t IasAudioRingBuffer::getHwPtrWrite() const
{
  if (mReal)
  {
    return mRingBufReal->getHwPtrWrite();
  }
  else
  {
    return 0;
  }
}

void IasAudioRingBuffer::resetFromWriter()
{
  if (mReal)
  {
    mRingBufReal->resetFromWriter();
  }
}

void IasAudioRingBuffer::resetFromReader()
{
  if (mReal)
  {
    mRingBufReal->resetFromReader();
  }
}

void IasAudioRingBuffer::setName(const std::string &name)
{
  mName = name;
}

const std::string& IasAudioRingBuffer::getName() const
{
  return mName;
}

void IasAudioRingBuffer::zeroOut()
{
  if (mReal)
  {
    mRingBufReal->zeroOut();
  }
}



} // namespace IasAudio
