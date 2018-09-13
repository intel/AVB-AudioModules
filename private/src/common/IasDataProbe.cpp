/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasDataProbe.cpp
 * @date   2016
 * @brief
 */

#include "internal/audio/common/IasAudioLogging.hpp"
#include "internal/audio/common/IasDataProbe.hpp"

#include "internal/audio/common/helper/IasCopyAudioAreaBuffers.hpp"
#include "audio/common/audiobuffer/IasMemoryAllocator.hpp"
#include "stdlib.h"
#include <algorithm>    // std::min

using namespace std;

namespace IasAudio {


#define DATA_PROBE_FORMAT_MASK 0xEFFF9

static const std::string cClassName = "IasDataProbe::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"


IasDataProbe::IasResult IasDataProbe::checkWaveFileInfos(SF_INFO &info,
                                                         uint32_t sampleRate,
                                                         IasAudioCommonDataFormat dataFormat)
{
  IasDataProbe::IasResult res = eIasOk;
  if(info.channels != 1)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "number of channels must be 1, channel number of",info.channels,"not supported");
    return eIasFailed;;
  }
  if( (info.format & DATA_PROBE_FORMAT_MASK) )
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "unsupported file format:", info.format);
    return eIasFailed;
  }
  if( info.samplerate != static_cast<int32_t>(sampleRate) )
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "sample rate does not match, in file:", info.samplerate,"requested:", sampleRate);
    return eIasFailed;
  }
  switch(dataFormat)
  {
    case eIasFormatInt16:
      if ( (info.format & 0x0000F) != SF_FORMAT_PCM_16)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "format of wav file:",info.format,"; it does not match requested SF_FORMAT_PCM_16");
        res = eIasFailed;
      }
      break;
    case eIasFormatInt32:
      if ((info.format & 0x0000F) != SF_FORMAT_PCM_32)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "format of wav file:",info.format,"; it does not match requested SF_FORMAT_PCM_32");
        res = eIasFailed;
      }
      break;
    default:
      if ((info.format & 0x0000F) != SF_FORMAT_FLOAT)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "format of wav file:",info.format,"; it does not match requested SF_FORMAT_PCM_FLOAT");
        res = eIasFailed;
      }
      break;
  }
  return res;
}

IasDataProbe::IasDataProbe()
:mLog(IasAudioLogging::registerDltContext("DP", "Data Probe"))
,mMode(eIasDataProbeInit)
,mStarted(false)
,mFileNamePrefix("")
,mDataFormat(eIasFormatUndef)
,mNumChannels(0)
,mStartIndex(0)
,mBufferSize(0)
,mIntermediateBuffer(nullptr)
,mArea(nullptr)
,mNumFramesToProcess(0)
,mem(nullptr)
{
  //Nothing to do here
}

IasDataProbe::~IasDataProbe()
{
  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, "Destructor");
  list<IasDataProbeWaveFile>::iterator it;

  for(it=mWaveFileList.begin(); it!= mWaveFileList.end(); it++)
  {
    if(it->file != nullptr)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "close file", it->name.c_str());
      sf_close(it->file);
    }
  }

  if(mem != nullptr && mIntermediateBuffer != nullptr)
  {
    mem->deallocate(mIntermediateBuffer);
    mIntermediateBuffer = nullptr;
    delete mem;
    mem = nullptr;
  }
  if(mArea != nullptr)
  {
    delete[] mArea;
    mArea = nullptr;
  }
}

void IasDataProbe::reset()
{
  list<IasDataProbeWaveFile>::iterator it;

  for(it=mWaveFileList.begin(); it!= mWaveFileList.end(); it++)
  {
    if(it->file != nullptr)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "close file", it->name.c_str());
      sf_close(it->file);
    }
  }

  if(mem != nullptr && mIntermediateBuffer != nullptr)
  {
    mem->deallocate(mIntermediateBuffer);
    mIntermediateBuffer = nullptr;
    delete mem;
    mem = nullptr;
  }
  if(mArea != nullptr)
  {
    delete[] mArea;
    mArea = nullptr;
  }
  mWaveFileList.clear();
  mDataFormat = eIasFormatUndef;
  mNumChannels = 0;
  mStartIndex = 0;
  mNumFramesToProcess = 0;
  mFileNamePrefix = "";
  mBufferSize = 0;
  atomic_thread_fence(memory_order_release);
  mStarted.store(false,memory_order_relaxed);
  mMode.store(eIasDataProbeInit,memory_order_relaxed);

}

IasDataProbe::IasResult IasDataProbe::startInject(const std::string &fileNamePrefix,
                                                  uint32_t numChannels,
                                                  uint32_t sampleRate,
                                                  IasAudioCommonDataFormat dataFormat,
                                                  uint32_t startIndex,
                                                  uint32_t bufferSize,
                                                  uint32_t numSeconds)
{
  IasDataProbeMode mode = mMode.load(memory_order_relaxed);
  if(mode == eIasDataProbeRecord)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Inject not allowed when in record mode");
    return eIasFailed;
  }
  bool bStarted = mStarted.load(memory_order_relaxed);
  if(bStarted == true)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Probing currently running, wait till finished");
    return eIasFailed;
  }
  atomic_thread_fence(memory_order_acquire);
  if (fileNamePrefix.empty() == true )
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: file name must not be an empty string");
    return eIasFailed;
  }
  mFileNamePrefix = fileNamePrefix;
  if (numChannels == 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "number of channels must no be zero");
    return eIasFailed;
  }
  if (dataFormat == eIasFormatUndef)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "undefined data format");
    return eIasFailed;
  }

  mNumFramesToProcess = numSeconds*sampleRate;

  for(uint32_t i=0; i<numChannels; i++)
  {
    std::string name = mFileNamePrefix;
    name.append("_ch");
    name.append(std::to_string(i));
    name.append(".wav");
    IasDataProbeWaveFile waveFile;
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "try to open", name.c_str());
    waveFile.file = sf_open(name.c_str(),SFM_READ,&waveFile.info);
    if(waveFile.file == nullptr)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "could not open", name.c_str());
      reset();
      return eIasFailed;
    }
    if(checkWaveFileInfos(waveFile.info,sampleRate,dataFormat))
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "verifying file infos failed for file:", name.c_str());
      sf_close(waveFile.file);
      reset();
      return eIasFailed;
    }
    sf_count_t samplesInFile = sf_seek(waveFile.file,0,SEEK_END);
    mNumFramesToProcess = std::min(mNumFramesToProcess,static_cast<uint32_t>(samplesInFile));
    samplesInFile = sf_seek(waveFile.file,0,SEEK_SET);
    waveFile.name = name;
    mWaveFileList.push_back(waveFile);
  }

  mStartIndex = startIndex;
  mNumChannels = numChannels;
  mBufferSize = bufferSize;
  mDataFormat = dataFormat;
  uint32_t sampleSize = toSize(mDataFormat);

  mem = new IasMemoryAllocator(mFileNamePrefix, mNumChannels*mBufferSize*sampleSize, false);
  IAS_ASSERT(mem != nullptr);
  IasAudioCommonResult memres = mem->init(IasMemoryAllocator::eIasCreate);
  IAS_ASSERT(memres == eIasResultOk);
  (void)memres;
  mem->allocate(32,mNumChannels*mBufferSize*sampleSize,&mIntermediateBuffer);
  IAS_ASSERT(mIntermediateBuffer != nullptr);

  mArea = new IasAudioArea[mNumChannels];
  IAS_ASSERT(mArea != nullptr);
  for(uint32_t i=0; i< mNumChannels; i++)
  {
    mArea[i].start = mIntermediateBuffer;
    mArea[i].index = i;
    mArea[i].first = i * mBufferSize * toSize(mDataFormat) * 8;
    mArea[i].maxIndex = mNumChannels-1;
    mArea[i].step = toSize(mDataFormat)*8;
  }
  std::atomic_thread_fence(std::memory_order_release);
  mMode.store(eIasDataProbeInject,memory_order_relaxed);
  mStarted.store(true,std::memory_order_relaxed);
  return eIasOk;
}

IasDataProbe::IasResult IasDataProbe::startRecording(const std::string &fileNamePrefix,
                                                     uint32_t numChannels,
                                                     uint32_t sampleRate,
                                                     IasAudioCommonDataFormat dataFormat,
                                                     uint32_t startIndex,
                                                     uint32_t bufferSize,
                                                     uint32_t numSeconds)
{

  IasResult res = eIasOk;

  IasDataProbeMode mode = mMode.load(memory_order_relaxed);
  if(mode == eIasDataProbeInject)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Recording not allowed when in inject mode");
    return eIasFailed;
  }
  bool bStarted = mStarted.load(memory_order_relaxed);
  if(bStarted == true)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Probing currently running, wait till finished");
    return eIasFailed;
  }
  atomic_thread_fence(memory_order_acquire);
  if (fileNamePrefix.empty() == true )
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: file name must not be an empty string");
    res = eIasFailed;
  }
  mFileNamePrefix = fileNamePrefix;
  if (numChannels == 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "number of channels must no be zero");
    return eIasFailed;
  }
  if (sampleRate == 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid parameter: sample rate must not be zero");
    res = eIasFailed;
  }
  int32_t fileFormat = SF_FORMAT_WAV;
  switch(dataFormat)
  {
    case eIasFormatFloat32:
      fileFormat |= SF_FORMAT_FLOAT;
      break;
    case eIasFormatInt32:
      fileFormat |= SF_FORMAT_PCM_32;
      break;
    case eIasFormatInt16:
      fileFormat |= SF_FORMAT_PCM_16;
      break;
    default:
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "dataFormat not defined");
      res = eIasFailed;
      break;
  }
  if(res != eIasOk)
  {
    return res;
  }

  for(uint32_t i=0; i<numChannels; i++)
  {
    std::string name = mFileNamePrefix;
    name.append("_ch");
    name.append(std::to_string(i));
    name.append(".wav");
    IasDataProbeWaveFile waveFile;
    waveFile.info.channels = 1;
    waveFile.info.samplerate = sampleRate;
    waveFile.info.format = fileFormat;

    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "try to open", name.c_str());
    waveFile.file = sf_open(name.c_str(),SFM_WRITE,&waveFile.info);
    if(waveFile.file == nullptr)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "could not open", name.c_str());
      reset();
      return eIasFailed;
    }
    waveFile.name = name;
    mWaveFileList.push_back(waveFile);
  }

  mStartIndex = startIndex;
  mNumChannels = numChannels;
  mBufferSize = bufferSize;
  mDataFormat = dataFormat;
  mNumFramesToProcess = numSeconds * sampleRate;

  uint32_t sampleSize = toSize(mDataFormat);
  mem = new IasMemoryAllocator(mFileNamePrefix, mNumChannels*mBufferSize*sampleSize, false);
  IAS_ASSERT(mem != nullptr);
  IasAudioCommonResult memres = mem->init(IasMemoryAllocator::eIasCreate);
  IAS_ASSERT(memres == eIasResultOk);
  (void)memres;
  mem->allocate(32,mNumChannels*mBufferSize*sampleSize,&mIntermediateBuffer);
  IAS_ASSERT(mIntermediateBuffer != nullptr);

  mArea = new IasAudioArea[mNumChannels];
  IAS_ASSERT(mArea != nullptr);
  for(uint32_t i=0; i< mNumChannels; i++)
  {
    mArea[i].start = mIntermediateBuffer;
    mArea[i].index = i;
    mArea[i].first = i * mBufferSize * toSize(mDataFormat) * 8;
    mArea[i].maxIndex = mNumChannels-1;
    mArea[i].step = toSize(mDataFormat)*8;
  }
  std::atomic_thread_fence(std::memory_order_release);
  mMode.store(eIasDataProbeRecord,memory_order_relaxed);
  mStarted.store(true,std::memory_order_relaxed);
  return res;
}

IasDataProbe::IasResult IasDataProbe::process(IasAudioArea* area,  uint32_t offset, uint32_t numFrames)
{
  IasDataProbe::IasResult res = eIasOk;
  bool bStarted = mStarted.load(memory_order_relaxed);

  if(bStarted == true)
  {
    if(numFrames > mBufferSize)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Probe buffer size is only", mBufferSize,", can't process", numFrames,"in one call");
      return eIasFailed;
    }
    IasDataProbeMode mode = mMode.load(memory_order_relaxed);
    atomic_thread_fence(std::memory_order_acquire);
    if (mode == eIasDataProbeInject)
    {
      res = injectData(area, offset, numFrames);
    }
    else
    {
      res = recordData(area, offset, numFrames);
    }
  }
  return res;
}

void IasDataProbe::updateFilePosition(int32_t numFrames)
{
  bool bStarted = mStarted.load(memory_order_relaxed);

  if(bStarted == true)
  {
      DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, "Update File position by", numFrames,"frames; framesToProcess before:",mNumFramesToProcess);
      std::list<IasDataProbeWaveFile>::iterator it;
      for (it=mWaveFileList.begin(); it!=mWaveFileList.end();it++)
      {
        sf_seek(it->file, numFrames, SEEK_CUR);
      }
      mNumFramesToProcess -= numFrames;
      DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, "Update File position by", numFrames,"frames; framesToProcess after:",mNumFramesToProcess);
      if(mNumFramesToProcess == 0)
      {
        reset();
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Probing finished, stopping now");
      }

  }
}

IasDataProbe::IasResult IasDataProbe::recordData(IasAudioArea* area, uint32_t offset, uint32_t numFrames )
{
  int32_t res = 0;

  if (mStarted)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, "called with",numFrames,"frames");
    copyAudioAreaBuffers(mArea,mDataFormat,0,mNumChannels,0,numFrames,area,mDataFormat,offset,mNumChannels,mStartIndex,numFrames);
    switch (mDataFormat)
    {
      case eIasFormatInt16:
        res = write_short(numFrames);
        break;
      case eIasFormatInt32:
        res = write_int32(numFrames);
        break;
      default:
        res = write_float32(numFrames);
        break;
    }
    if (res != static_cast<int32_t>(numFrames))
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error on writing to file, stopping now");
      reset();
      return eIasFailed;
    }
    else
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, "Frames still to process:", mNumFramesToProcess);
      mNumFramesToProcess -= numFrames;
      if(mNumFramesToProcess < numFrames)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Record finished, stopping now");
        reset();
        return eIasFinished;
      }
    }
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "No record operation was started");
    return eIasNoOp;
  }

  return eIasOk;
}

IasDataProbe::IasResult IasDataProbe::injectData(IasAudioArea* area, uint32_t offset, uint32_t numFrames)
{
  int32_t numFramesReadFromFile = 0;
  IasDataProbe::IasResult res = eIasOk;

  if (mStarted)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, "called with",numFrames,"frames");
    switch (mDataFormat)
    {
      case eIasFormatInt16:
        numFramesReadFromFile = read_short(numFrames);
        break;
      case eIasFormatInt32:
        numFramesReadFromFile = read_int32(numFrames);
        break;
      default:
        numFramesReadFromFile = read_float32(numFrames);;
        break;
    }
    if (numFramesReadFromFile == static_cast<int32_t>(numFrames))
    {
      copyAudioAreaBuffers(area,mDataFormat,offset,mNumChannels,mStartIndex,numFrames,mArea,mDataFormat,0,mNumChannels,0,numFrames);
      DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, "Frames still to process:", mNumFramesToProcess);
      mNumFramesToProcess -= numFrames;
      if(mNumFramesToProcess< numFrames)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Inject finished, stopping now");
        reset();
        return eIasFinished;
      }
    }
    else
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Could not read desired number of frames, assuming file end");
      reset();
      res = eIasFinished;
    }
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "No inject operation was started");
    return eIasNoOp;
  }

  return res;
}

void IasDataProbe::stop()
{
  bool bStarted = mStarted.load(std::memory_order_relaxed);
  if (bStarted == true)
  {
    atomic_thread_fence(memory_order_acquire);
    reset();
  }
}

int32_t IasDataProbe::read_short(uint32_t numFrames)
{
  sf_count_t numFramesRead = 0;
  std::list<IasDataProbeWaveFile>::iterator it;
  uint32_t cnt = 0;
  for (it=mWaveFileList.begin(); it!=mWaveFileList.end();it++)
  {
    numFramesRead = sf_readf_short(it->file,
                                   reinterpret_cast<int16_t*>((char*)mArea[cnt].start + (mArea[cnt].first>>3)),
                                   numFrames);
    if(numFramesRead != numFrames)
    {
      break;
    }
    cnt++;
  }
  return static_cast<int32_t>(numFramesRead);
}

int32_t IasDataProbe::write_short(uint32_t numFrames)
{
  sf_count_t numFramesWritten = 0;
  std::list<IasDataProbeWaveFile>::iterator it;
  uint32_t cnt = 0;
  for (it=mWaveFileList.begin(); it!=mWaveFileList.end();it++)
  {
    numFramesWritten = sf_writef_short(it->file,
                                       reinterpret_cast<int16_t*>((char*)mArea[cnt].start + (mArea[cnt].first>>3)),
                                       numFrames);
    if(numFramesWritten != numFrames)
    {
      break;
    }
    cnt++;
  }
  return static_cast<int32_t>(numFramesWritten);
}

int32_t IasDataProbe::write_int32(uint32_t numFrames)
{
  sf_count_t numFramesWritten = 0;
  std::list<IasDataProbeWaveFile>::iterator it;
  uint32_t cnt = 0;
  for (it=mWaveFileList.begin(); it!=mWaveFileList.end();it++)
  {
    numFramesWritten = sf_writef_int(it->file,
                                       reinterpret_cast<int32_t*>((char*)mArea[cnt].start + (mArea[cnt].first>>3)),
                                       numFrames);
    if(numFramesWritten != numFrames)
    {
      break;
    }
    cnt++;
  }
  return static_cast<int32_t>(numFramesWritten);
}

int32_t IasDataProbe::read_int32(uint32_t numFrames)
{
  sf_count_t numFramesRead = 0;
  std::list<IasDataProbeWaveFile>::iterator it;
  uint32_t cnt = 0;
  for (it=mWaveFileList.begin(); it!=mWaveFileList.end();it++)
  {
    numFramesRead = sf_readf_int(it->file,
                                 reinterpret_cast<int32_t*>((char*)mArea[cnt].start + (mArea[cnt].first>>3)),
                                 numFrames);
    if(numFramesRead != numFrames)
    {
      break;
    }
    cnt++;
  }
  return static_cast<int32_t>(numFramesRead);
}

int32_t IasDataProbe::write_float32(uint32_t numFrames)
{
  sf_count_t numFramesWritten = 0;
  std::list<IasDataProbeWaveFile>::iterator it;
  uint32_t cnt = 0;
  for (it=mWaveFileList.begin(); it!=mWaveFileList.end();it++)
  {
    numFramesWritten = sf_writef_float(it->file,
                                       reinterpret_cast<float*>((char*)mArea[cnt].start + (mArea[cnt].first>>3)),
                                       numFrames);
    if(numFramesWritten != numFrames)
    {
      break;
    }
    cnt++;
  }
  return static_cast<int32_t>(numFramesWritten);
}

int32_t IasDataProbe::read_float32(uint32_t numFrames)
{
  sf_count_t numFramesRead = 0;
  std::list<IasDataProbeWaveFile>::iterator it;
  uint32_t cnt = 0;
  for (it=mWaveFileList.begin(); it!=mWaveFileList.end();it++)
  {
    numFramesRead = sf_readf_float(it->file,
                                 reinterpret_cast<float*>((char*)mArea[cnt].start + (mArea[cnt].first>>3)),
                                 numFrames);
    if(numFramesRead != numFrames)
    {
      break;
    }
    cnt++;
  }
  return static_cast<int32_t>(numFramesRead);
}


#define STRING_RETURN_CASE(name) case name: return std::string(#name); break
#define DEFAULT_STRING(name) default: return std::string(name)
std::string toString(const IasDataProbe::IasResult& type)
{
  switch(type)
  {
    STRING_RETURN_CASE(IasDataProbe::eIasOk);
    STRING_RETURN_CASE(IasDataProbe::eIasFailed);
    STRING_RETURN_CASE(IasDataProbe::eIasFinished);
    STRING_RETURN_CASE(IasDataProbe::eIasNoOp);
    STRING_RETURN_CASE(IasDataProbe::eIasAlreadyStarted);
    DEFAULT_STRING("Invalid IasDataProbe::IasResult => " + std::to_string(type));
  }
}

std::string toString(const IasProbingAction& action)
{
  switch(action)
    {
      STRING_RETURN_CASE(eIasProbingStart);
      STRING_RETURN_CASE(eIasProbingStop);
      DEFAULT_STRING("Invalid IasProbingAction => " + std::to_string(action));
    }
}

}
