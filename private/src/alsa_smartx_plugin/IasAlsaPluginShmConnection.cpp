/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file IasAlsaPluginShmConnection.cpp
 * @brief Defines a structure that contains all the modules that
 * are needed for a connection between the smartx and the smartx Plugin.
 */

#include <string>

#include "internal/audio/common/audiobuffer/IasAudioRingBuffer.hpp"
#include "internal/audio/common/audiobuffer/IasAudioRingBufferFactory.hpp"
#include "audio/common/audiobuffer/IasMemoryAllocator.hpp"

#include "internal/audio/common/alsa_smartx_plugin/IasAlsaPluginShmConnection.hpp"
#include "internal/audio/common/alsa_smartx_plugin/IasAlsaHwConstraintsStatic.hpp"
#include "internal/audio/common/alsa_smartx_plugin/IasAlsaPluginIpc.hpp"

#include "internal/audio/common/IasAudioLogging.hpp"
#include "internal/audio/common/IasIntProcMutex.hpp"

// This include has to be the last in the list of includes else there will be
// compile errors in combination with boost. The problem seems to be related with
// the definition of the symbol __need_timeval that shouldn't be defined when including
// the boost libraries
#include <alsa/asoundlib.h>


namespace IasAudio {

static const std::string cClassName = "IasAlsaPluginShmConnection::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"
#define LOG_DEVICE "device=" + connectionName + ":"


IasAlsaPluginShmConnection::IasAlsaPluginShmConnection()
  :mLog(IasAudioLogging::registerDltContext("SHM", "Shm Connection"))
  ,mInIpc(nullptr)
  ,mOutIpc(nullptr)
  ,mConstraints(nullptr)
  ,mRingBufferName()
  ,mRingBuffer(nullptr)
  ,mUpdateAvailable(nullptr)
  ,mAllocator(nullptr)
  ,mIsCreator(false)
  ,mFdSignal()
  ,mConnectionName()
  ,mGroupName()
  ,mOpenOnceMutex(nullptr)
{
}

IasAlsaPluginShmConnection::~IasAlsaPluginShmConnection()
{
  if(mAllocator && mIsCreator)
  {
    delete mAllocator;

    IasAudioRingBufferFactory::getInstance()->destroyRingBuffer(mRingBuffer);
  }
  if(mIsCreator)
  {
    mFdSignal.close();
    mFdSignal.destroy();
  }
}



IasAudioCommonResult IasAlsaPluginShmConnection::createConnection(const std::string& connectionName, const std::string& groupName)
{
  if (connectionName.empty())
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Connection name may not be an empty string");
    return eIasResultInvalidParam;
  }
  mConnectionName = connectionName;
  mGroupName = groupName;

  // This class can only handle one connection. Second call will fail.
  if(mAllocator)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Connection already created");
    return eIasResultObjectAlreadyExists;
  }
  // Mark Class as creator, that the content gets deleted after use
  mIsCreator = true;

  IasAudioCommonResult result(eIasResultOk);

  // Save Ringbuffer name for later creation or recreation
  mRingBufferName = connectionName + "_ringbuffer";

  // Calculate the Size of the needed shared memory
  uint32_t totalSize = static_cast<uint32_t>(2 * sizeof(IasAudioIpc) +
                                                   sizeof(IasAlsaHwConstraintsStatic) +
                                                   2 * sizeof(int32_t) +
                                                   sizeof(bool) +
                                                   sizeof(IasIntProcMutex));

  // Get an allocator
  mAllocator = new IasMemoryAllocator(connectionName+"_connection", totalSize ,true);
  if(!mAllocator)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Out of memory");
    return eIasResultMemoryError;
  }

  // Init the shared memory
  if( eIasResultOk !=
    (result = mAllocator->init(IasMemoryAllocator::eIasCreate)))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Fail to init the allocator: error=", toString(result));
    return result;
  }

  // Change group of the shared memory file
  std::string errorMsg = "";
  if( eIasResultOk !=
    (result = mAllocator->changeGroup(mGroupName, &errorMsg)))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, errorMsg);
    return result;
  }

  // Allocate space for the constraints
  if( eIasResultOk !=
    (result = mAllocator->allocate<IasAlsaHwConstraintsStatic>(connectionName + "_constraints", 1, &mConstraints)))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Fail to create a Constraints: error=", toString(result));
    return result;
  }

  // Allocate space for the Update flag
  if( eIasResultOk !=
    (result = mAllocator->allocate<bool>(connectionName + "_updateflag", 1, &mUpdateAvailable)))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Fail to create a update flag: error=", toString(result));
    return result;
  }
  // Init update flag status
  *mUpdateAvailable = true;

  IasAudioIpc* tempIpc;

  // Allocate space for the IPC queues
  if( eIasResultOk !=
    (result = mAllocator->allocate<IasAudioIpc>(connectionName + "_ipc", 2, &tempIpc)))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Fail to create a IpcConnection: error=", toString(result));
    return result;
  }
  // Save the pointer in class
  mInIpc = tempIpc + 1;
  mOutIpc = tempIpc;

  // Allocate space for the open once mutex
  if( eIasResultOk !=
    (result = mAllocator->allocate<IasIntProcMutex>(connectionName + "_openonce", 1, &mOpenOnceMutex)))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Fail to create open once mutex: error=", toString(result));
    return result;
  }

  return result;
}

IasAudioCommonResult IasAudio::IasAlsaPluginShmConnection::createRingBuffer(const IasAudioDeviceParamsPtr configStruct)
{
  if(!mIsCreator)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Only the server is allowed to create a ring buffer!");
    return eIasResultNotAllowed;
  }

  if(!mAllocator)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Not initialized");
    return eIasResultNotInitialized;
  }

  IasAudioCommonResult result(eIasResultOk);

  // Destroy Ringbuffer
  *mUpdateAvailable = false;

  if(mRingBuffer)
  {
    IasAudioRingBufferFactory::getInstance()->destroyRingBuffer(mRingBuffer);
  }

  // Create Ringbuffer
  result = IasAudioRingBufferFactory::getInstance()->
    createRingBuffer(&mRingBuffer, configStruct->periodSize,
                     configStruct->numPeriods, configStruct->numChannels,
                     configStruct->dataFormat, eIasRingBufferShared,
                     mRingBufferName,
                     mGroupName);
  if (result == eIasResultOk)
  {
    // Ringbuffer successfully created, now create the fdsignal
    IasFdSignal::IasResult fdres = mFdSignal.create(mConnectionName, mGroupName);
    if (fdres == IasFdSignal::eIasOk)
    {
      fdres = mFdSignal.open(mConnectionName, IasFdSignal::eIasWrite);
    }

    if (fdres == IasFdSignal::eIasOk)
    {
      // Find out if we are a playback or a capture device.
      // We can do this by using the naming convention of the connection
      // This has to be either smartx_XYZ_c or avb_XYZ_c for capture devices
      // or smartx_XYZ_p or avb_XYZ_p for playback devices
      IAS_ASSERT(mConnectionName.length() >= 1);
      char lastChar = mConnectionName[mConnectionName.length()-1];
      IasDeviceType deviceType;
      if (lastChar == 'c')
      {
        deviceType = eIasDeviceTypeSink;
      }
      else if (lastChar == 'p')
      {
        deviceType = eIasDeviceTypeSource;
      }
      else
      {
        deviceType = eIasDeviceTypeUndef;
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Connection name is not set up correctly. Should contain c(apture) or p(layback) as last character:", mConnectionName);
        result = eIasResultInitFailed;
      }
      mRingBuffer->setFdSignal(&mFdSignal, deviceType);
    }
    else
    {
      result = eIasResultInitFailed;
    }
  }

  *mUpdateAvailable = true;
  return result;
}


IasAudioCommonResult IasAlsaPluginShmConnection::findConnection(const std::string& connectionName)
{
  // This class can only handle one connection. Second call will fail.
  if(mAllocator)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Connection already found");
    return eIasResultObjectAlreadyExists;
  }

  IasAudioCommonResult result(eIasResultOk);

  // Set the name of the associated Ringbuffer
  mRingBufferName = connectionName + "_ringbuffer";

  // Calculate total size
  uint32_t totalSize = static_cast<uint32_t>(2 * sizeof(IasAudioIpc) +
                                                   sizeof(IasAlsaHwConstraintsStatic) +
                                                   2 * sizeof(int32_t) +
                                                   sizeof(bool) +
                                                   sizeof(IasIntProcMutex));

  // Get an allocator
  mAllocator = new IasMemoryAllocator(connectionName+"_connection", totalSize ,true);
  IAS_ASSERT(mAllocator != nullptr);

  // Try to find an instance of the allocator
  if( eIasResultOk !=
    (result = mAllocator->init(IasMemoryAllocator::eIasConnect)))
  {
    /**
     * @log <NAME> cannot be found in shared memory.
     */
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Fail to init the allocator: error=", toString(result));
    return result;
  }

  // Try to find the hardware constraints
  uint32_t dumpCount;

  if( eIasResultOk !=
    (result = mAllocator->find<IasAlsaHwConstraintsStatic>(connectionName + "_constraints", &dumpCount, &mConstraints)))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Fail to find Constraints: error=", toString(result));
    return result;
  }

  // Allocate space for the Update flag
  if( eIasResultOk !=
    (result = mAllocator->find<bool>(connectionName + "_updateflag", &dumpCount, &mUpdateAvailable)))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Fail to find the update flag: error=", toString(result));
    return result;
  }

  // Try to find the IPC queues
  IasAudioIpc* tempIpc = nullptr;
  uint32_t ipcNumber = 0;

  if( eIasResultOk !=
    (result = mAllocator->find<IasAudioIpc>(connectionName + "_ipc", &ipcNumber, &tempIpc)))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Fail to find a IpcConnection: error=", toString(result));
    return result;
  }

  if(ipcNumber != 2)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "IPC Count missmatch in shm.");
    return eIasResultNotInitialized;
  }

  // Save the IPC queues.
  mInIpc = tempIpc;
  mOutIpc = tempIpc + 1;

  // Try to find the open once mutex
  if( eIasResultOk !=
    (result = mAllocator->find<IasIntProcMutex>(connectionName + "_openonce", &dumpCount, &mOpenOnceMutex)))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Fail to find the open once mutex: error=", toString(result));
    return result;
  }

  return result;
}

IasAudioRingBuffer* IasAlsaPluginShmConnection::verifyAndGetRingBuffer()
{
  if(*mUpdateAvailable || !mRingBuffer)
  {
    // Get the factory
    IasAudioRingBufferFactory* factory = IasAudioRingBufferFactory::getInstance();
    if(!factory)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Ringbuffer Factory can't be reached.");
      return nullptr;
    }

    // Find an instance from the sample buffer
    mRingBuffer = factory->findRingBuffer(mRingBufferName);

    if(nullptr == mRingBuffer)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Fail to find a Ringbuffer.");
    }
    else
    {
      *mUpdateAvailable = false;
    }
  }

  return mRingBuffer;
}

#define STRING_RETURN_CASE(name) case name: return std::string(#name); break
#define DEFAULT_STRING(name) default: return std::string(name)

__attribute__ ((visibility ("default"))) std::string toString(const IasAudioIpcPluginControl &type)
{
  switch(type)
  {
    STRING_RETURN_CASE(eIasAudioIpcInvalid);
    STRING_RETURN_CASE(eIasAudioIpcNAK);
    STRING_RETURN_CASE(eIasAudioIpcACK);
    STRING_RETURN_CASE(eIasAudioIpcGetLatency);
    STRING_RETURN_CASE(eIasAudioIpcStart);
    STRING_RETURN_CASE(eIasAudioIpcPause);
    STRING_RETURN_CASE(eIasAudioIpcResume);
    STRING_RETURN_CASE(eIasAudioIpcStop);
    STRING_RETURN_CASE(eIasAudioIpcDrain);
    STRING_RETURN_CASE(eIasAudioIpcParameters);
    DEFAULT_STRING("Invalid IasAudioIpcPluginControl => " + std::to_string(type));
  }
}


}
