/*
 * @COPYRIGHT_TAG@
 */
/**
 * @file   IasAudioRingBufferFactory.cpp
 * @date   2015
 * @brief
 */
#include <sys/types.h>
#include <sys/socket.h>


#include "internal/audio/common/audiobuffer/IasAudioRingBuffer.hpp"
#include "internal/audio/common/audiobuffer/IasAudioRingBufferReal.hpp"
#include "internal/audio/common/audiobuffer/IasAudioRingBufferMirror.hpp"
#include "internal/audio/common/audiobuffer/IasAudioRingBufferFactory.hpp"
#include "internal/audio/common/audiobuffer/IasAudioRingBufferResult.hpp"
#include "audio/common/audiobuffer/IasMemoryAllocator.hpp"
#include "audio/common/audiobuffer/IasMetaDataFactory.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"

namespace IasAudio
{

static const std::string cClassName = "IasAudioRingBufferFactory::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"
#define LOG_BUFFER "buffer=" + name + ":"

IasAudioRingBufferFactory*  IasAudioRingBufferFactory::getInstance()
{
  static IasAudioRingBufferFactory theInstance;
  return &theInstance;
}


IasAudioRingBufferFactory::IasAudioRingBufferFactory()
  :mMemoryMap()
  ,mLog(IasAudioLogging::registerDltContext("ARF", "Audio Ringbuffer Factory"))
{
  //Nothing to do here
}

IasAudioRingBufferFactory::~IasAudioRingBufferFactory()
{
  //Nothing to do here
}

void IasAudioRingBufferFactory::destroyRingBuffer(IasAudioRingBuffer* ringBuf)
{
  IasMemoryAllocatorMap::iterator it = mMemoryMap.find(ringBuf);
  if(it != mMemoryMap.end())
  {
    IasMemoryAllocator* mem = (*it).second;
    const IasAudioRingBufferMirror* mir =  ringBuf->getMirror();
    if (mir != NULL)
    {
      mem->deallocate(mir);
    }
    const IasAudioRingBufferReal* real = ringBuf->getReal();
    if(real != NULL)
    {
      mem->deallocate(real);
    }
    delete (*it).first;
    delete (*it).second;
    mMemoryMap.erase(it);
  }
}

IasAudioRingBuffer* IasAudioRingBufferFactory::findRingBuffer(std::string name)
{
  IasAudioCommonResult res = eIasResultOk;

  IasMemoryAllocator* mem = new IasMemoryAllocator(name, 0, true);
  IAS_ASSERT(mem != nullptr);

  res = mem->init(IasMemoryAllocator::eIasConnect);
  if(res != eIasResultOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_BUFFER, "Unable to connect to shared memory:", toString(res));
    delete mem;
    return nullptr;
  }
  uint32_t numItems = 0;
  IasAudioRingBufferReal* ringBufReal = nullptr;
  std::string ringBufName = name +"_ringBufferReal";

  res = mem->find(ringBufName,&numItems,&ringBufReal);

  if(res != eIasResultOk || numItems != 1 || ringBufReal == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_BUFFER, "ringBufReal not found");
    delete mem;
    return NULL;
  }
  else
  {
    IasAudioRingBuffer* ringBuf = new IasAudioRingBuffer();
    IAS_ASSERT(ringBuf != nullptr);

    IasAudioRingBufferResult rbres = ringBuf->setup(ringBufReal);
    if(rbres != eIasRingBuffOk)
    {
      delete ringBuf;
      delete mem;
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_BUFFER, "setup of ringbuffer failed");
      return nullptr;
    }
    ringBuf->setName(name);

    std::pair<IasAudioRingBuffer*,IasMemoryAllocator*> tmpPair(ringBuf,mem);
    mMemoryMap.insert(tmpPair);
    return ringBuf;
  }
}

void IasAudioRingBufferFactory::loseRingBuffer(IasAudioRingBuffer* ringBuf)
{
  IasMemoryAllocatorMap::iterator it = mMemoryMap.find(ringBuf);
  if(it != mMemoryMap.end())
  {
    std::string name = ringBuf->getName();
    delete (*it).first;
    delete (*it).second;
    mMemoryMap.erase(it);
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_BUFFER, "Successfully deleted");
  }
}

IasAudioCommonResult IasAudioRingBufferFactory::createRingBuffer(IasAudioRingBuffer** ringbuffer,
                                                                 uint32_t periodSize,
                                                                 uint32_t numPeriods,
                                                                 uint32_t numChannels,
                                                                 IasAudioCommonDataFormat dataFormat,
                                                                 IasRingbufferType type,
                                                                 std::string name,
                                                                 std::string groupName)
{
  IasAudioCommonResult res = eIasResultOk;
  int32_t sampleSize=0;
  bool memAllocatorShared = false;

  uint32_t totalMemorySize = 0;
  sampleSize = toSize(dataFormat);
  if (name.empty() == true)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Name of ringbuffer may not be an empty string");
    return eIasResultInvalidParam;
  }
  else if(sampleSize == -1)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_BUFFER, "Invalid data format", static_cast<uint32_t>(dataFormat));
    return eIasResultInvalidSampleSize;
  }
  else if (periodSize == 0 && type != eIasRingBufferLocalMirror)
  {
    /**
     * @log The initialization parameter periodSize of the IasAudioDeviceParams structure for device with name <NAME> is invalid.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_BUFFER, "Period size must be greater than zero", periodSize);
    return eIasResultInvalidParam;
  }
  else if (numPeriods == 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_BUFFER, "Number periods must be greater than zero");
    return eIasResultInvalidParam;
  }
  else if (numChannels == 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_BUFFER, "Number channels must be greater than zero");
    return eIasResultInvalidParam;
  }

  uint32_t ringBufSizeReal =  static_cast<uint32_t>(sizeof(IasAudioRingBufferReal));
  uint32_t ringBufSizeMirror =  static_cast<uint32_t>(sizeof(IasAudioRingBufferMirror));
  uint32_t metaDataSize = IasMetaDataFactory::getSize(numPeriods);
  uint32_t memDataBuffer = sampleSize * numChannels * numPeriods * periodSize;

  bool allocateDataMem = true;

  if(ringbuffer == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_BUFFER, "Invalid parameter: ringbuffer == nullptr");
    return eIasResultInvalidParam;
  }

  switch (type)
  {
    case eIasRingBufferShared:
      memAllocatorShared = true;
      ringBufSizeMirror = 0;
      break;
    case eIasRingBufferLocalReal:
      memAllocatorShared = false;
      ringBufSizeMirror = 0;
      break;
    case eIasRingBufferLocalMirror:
      memAllocatorShared = false;
      memDataBuffer = 0;
      allocateDataMem = false;
      ringBufSizeReal = 0;
      break;
    case eIasRingBufferUndef:
    default:
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_BUFFER, "Invalid ringbuffer type:", static_cast<uint32_t>(type));
      return eIasResultInvalidParam;
  }

  totalMemorySize = memDataBuffer + ringBufSizeReal + ringBufSizeMirror + metaDataSize;
  IasMemoryAllocator* mem = new IasMemoryAllocator(name, totalMemorySize, memAllocatorShared);
  IAS_ASSERT(mem != nullptr);

  res = mem->init(IasMemoryAllocator::eIasCreate);
  if (res != eIasResultOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_BUFFER, "Error initializing the memory allocator:", toString(res));
    delete mem;
    return res;
  }
  if (memAllocatorShared == true)
  {
    // Change group of shared memory file
    std::string errorMsg = "";
    res = mem->changeGroup(groupName, &errorMsg);
    if (res != eIasResultOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_BUFFER, errorMsg);
      delete mem;
      return res;
    }
  }

  void* dataBuf = nullptr;
  if(allocateDataMem == true)
  {

    res = mem->allocate(16,memDataBuffer,&dataBuf);
    if(res != eIasResultOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_BUFFER, "Error allocating the data memory:", toString(res));
      delete mem;
      return res;
    }
  }


  IasAudioRingBuffer* ringBuf = new IasAudioRingBuffer();
  IAS_ASSERT(ringBuf != nullptr);
  IasMetaDataFactory *myMetaDataFactory = new IasMetaDataFactory(mem);
  IAS_ASSERT(myMetaDataFactory != nullptr);

  std::string metaDataName = name +"_metaData";
  IasMetaData* metaData = nullptr;
  res = myMetaDataFactory->create(metaDataName,numPeriods,&metaData);
  if(res != eIasResultOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_BUFFER, "Error creating meta data:", toString(res));
    delete ringBuf;
    delete mem;
    delete myMetaDataFactory;
    return res;
  }
  IasAudioRingBufferResult ringBufRes = eIasRingBuffOk;
  if(eIasRingBufferLocalMirror != type)
  {
    IasAudioRingBufferReal* ringBufReal;
    std::string ringBufNameReal = name +"_ringBufferReal";
    res = mem->allocate<IasAudioRingBufferReal>(ringBufNameReal,1,&ringBufReal);

    if(res != eIasResultOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_BUFFER, "Error allocating real ringbuffer:", toString(res));
      delete ringBuf;
      delete mem;
      delete myMetaDataFactory;
      return res;
    }

    if(type == eIasRingBufferLocalReal)
    {
      ringBufRes = ringBuf->init(periodSize,
                                 numPeriods,
                                 numChannels,
                                 dataFormat,
                                 dataBuf,
                                 memAllocatorShared,
                                 metaData,
                                 ringBufReal);
    }
    else
    {
      ringBufRes = ringBuf->init(periodSize,
                                 numPeriods,
                                 numChannels,
                                 dataFormat,
                                 dataBuf,
                                 memAllocatorShared,
                                 metaData,
                                 ringBufReal);
    }
  }
  else
  {
    IasAudioRingBufferMirror* ringBufMirror;
    std::string ringBufNameMirror = name +"_ringBufferMirror";
    res = mem->allocate<IasAudioRingBufferMirror>(ringBufNameMirror,1,&ringBufMirror);

    if(res != eIasResultOk)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_BUFFER, "Error allocating mirror ringbuffer:", toString(res));
      delete ringBuf;
      delete mem;
      delete myMetaDataFactory;
      return res;
    }
    ringBufRes = ringBuf->init(numChannels,
                               ringBufMirror);
  }

  if(ringBufRes != eIasRingBuffOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_BUFFER, "Error initializing ringbuffer:", toString(res));
    delete ringBuf;
    delete mem;
    delete myMetaDataFactory;
    return eIasResultInitFailed;
  }

  std::pair<IasAudioRingBuffer*,IasMemoryAllocator*> tmpPair(ringBuf,mem);

  ringBuf->setName(name);

  *ringbuffer = ringBuf;
  mMemoryMap.insert(tmpPair);

  delete myMetaDataFactory;
  return res;
}

}
