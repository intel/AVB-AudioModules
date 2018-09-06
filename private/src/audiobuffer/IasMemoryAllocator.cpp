/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasMemoryAllocator.cpp
 * @date   2015
 * @brief
 */

#include <sys/types.h>
#include <grp.h>
#include <iostream>
#include <boost/system/error_code.hpp>
#include <boost/filesystem.hpp>
#include "audio/common/audiobuffer/IasMemoryAllocator.hpp"
#include "audio/common/audiobuffer/IasMetaData.hpp"

using namespace boost::interprocess;
using namespace boost::filesystem;

namespace IasAudio {

#ifndef SHM_ROOT_PATH
#define SHM_ROOT_PATH  "/dev/shm/"
#endif
const std::string cShmRoot = SHM_ROOT_PATH;

IasMemoryAllocator::IasMemoryAllocator(const std::string &name, uint32_t totalMemorySize, bool shared)
  :mInitialized(false)
  ,mName(name)
  ,mTotalMemorySize(totalMemorySize)
  ,mShared(shared)
  ,mFullName(name)
  ,mManaged_shm(NULL)
  ,mManaged_heap(NULL)
{
  //Nothing to do here
}

IasMemoryAllocator::~IasMemoryAllocator()
{
  if (mInitialized == true)
  {
    if (mShared == true)
    {
      shared_memory_object::remove(mFullName.c_str());
    }
    delete mManaged_heap;
    delete mManaged_shm;
  }
}

IasAudioCommonResult IasMemoryAllocator::init(IasOpenFlag flag)
{
  if (mInitialized == false)
  {
    uint32_t pageSize = static_cast<uint32_t>(mapped_region::get_page_size());
    // add one page of memory to accommodate the housekeeping overhead
    if (mShared == false)
    {
      if (flag == eIasCreate)
      {
        // Allocate everything in heap
        managed_heap_memory::size_type totalSize = mTotalMemorySize + pageSize;
        try
        {
          mManaged_heap = new managed_heap_memory(totalSize);
        }
        catch(char* e)
        {
          (void) e;
          return eIasResultMemoryError;
        }
      }
      else
      {
        return eIasResultInvalidParam;
      }
    }
    else
    {
      if (flag == eIasCreate)
      {
        shared_memory_object::remove(mFullName.c_str());
        managed_shared_memory::size_type totalSize = mTotalMemorySize + pageSize;
        try
        {
          mManaged_shm = new managed_shared_memory(create_only, mFullName.c_str(), totalSize);
        }
        catch(char* e)
        {
          (void) e;
          return eIasResultMemoryError;
        }
      }
      else
      {
        std::string absPath = cShmRoot + mFullName;
        if (exists(absPath) == true)
        {
          try
          {
            mManaged_shm = new managed_shared_memory(open_only, mFullName.c_str());
          }
          catch(char* e)
          {
            (void) e;
            return eIasResultMemoryError;
          }
        }
        else
        {
          return eIasResultInvalidShmPath;
        }
      }
    }
    mInitialized = true;
  }
  return eIasResultOk;
}

IasAudioCommonResult IasMemoryAllocator::allocate(uint32_t alignment, uint32_t size, void **buffer)
{
  if (mShared == false)
  {
    return allocateHeap(alignment, size, buffer);
  }
  else
  {
    return allocateShared(alignment, size, buffer);
  }
}

void IasMemoryAllocator::deallocate(void *buffer)
{
  if (mShared == false)
  {
    deallocateHeap(buffer);
  }
  else
  {
    deallocateShared(buffer);
  }
}

IasAudioCommonResult IasMemoryAllocator::allocateShared(uint32_t alignment, uint32_t size, void **buffer)
{
  if (mInitialized == false)
  {
    return eIasResultNotInitialized;
  }
  if (buffer == NULL)
  {
    return eIasResultInvalidParam;
  }
  managed_shared_memory::size_type sizeForAllocation = size /*- managed_shared_memory::PayloadPerAllocation*/;
  // Subtract the payload from the requested size for better memory usage. See boost documentation for detailed info.
  void *ptr = mManaged_shm->allocate_aligned(sizeForAllocation, static_cast<managed_shared_memory::size_type>(alignment), std::nothrow);
  if (ptr != NULL)
  {
    *buffer = ptr;
    return eIasResultOk;
  }
  else
  {
    return eIasResultMemoryError;
  }
}

void IasMemoryAllocator::deallocateShared(void *buffer)
{
  if (mInitialized == true)
  {
    mManaged_shm->deallocate(buffer);
  }
}

IasAudioCommonResult IasMemoryAllocator::allocateHeap(uint32_t alignment, uint32_t size, void **buffer)
{
  if (mInitialized == false)
  {
    return eIasResultNotInitialized;
  }
  if (buffer == NULL)
  {
    return eIasResultInvalidParam;
  }
  managed_heap_memory::size_type sizeForAllocation = size;
  void *ptr = mManaged_heap->allocate_aligned(sizeForAllocation, static_cast<managed_heap_memory::size_type>(alignment), std::nothrow);
  if (ptr != NULL)
  {
    *buffer = ptr;
    return eIasResultOk;
  }
  else
  {
    return eIasResultMemoryError;
  }
}

void IasMemoryAllocator::deallocateHeap(void *buffer)
{
  if (mInitialized == true)
  {
    mManaged_heap->deallocate(buffer);
  }
}

IasAudioCommonResult IasMemoryAllocator::getFreeMemory(uint32_t *freeMem) const
{
  if (mInitialized == false)
  {
    return eIasResultNotInitialized;
  }
  if (freeMem == NULL)
  {
    return eIasResultInvalidParam;
  }
  if (mShared == false)
  {
    *freeMem = static_cast<uint32_t>(mManaged_heap->get_free_memory());
  }
  else
  {
    *freeMem = static_cast<uint32_t>(mManaged_shm->get_free_memory());
  }
  return eIasResultOk;
}

IasAudioCommonResult IasMemoryAllocator::changeGroup(const std::string &groupName, std::string *errorMsg)
{
  struct group* groupInfo = getgrnam(groupName.c_str());
  if (groupInfo == nullptr)
  {
    if (errorMsg != nullptr)
    {
      *errorMsg = "Group " + groupName + " does not exist. Shared memory owner cannot be changed accordingly.";
    }
    return eIasResultInvalidParam;
  }
  std::string fullShmPath = cShmRoot + mFullName;
  int err = chown(fullShmPath.c_str(), -1, groupInfo->gr_gid);
  if (err < 0)
  {
    if (errorMsg != nullptr)
    {
      *errorMsg = "Unable to change group of " + fullShmPath + " to " + groupName + ": " + strerror(errno);
    }
    return eIasResultInvalidParam;
  }
  err = chmod(fullShmPath.c_str(), S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
  if (err < 0)
  {
    if (errorMsg != nullptr)
    {
      *errorMsg = "Unable to change mode bits of " + fullShmPath + ": " + strerror(errno);
    }
    return eIasResultNotAllowed;
  }
  return eIasResultOk;
}


} // namespace IasAudio
