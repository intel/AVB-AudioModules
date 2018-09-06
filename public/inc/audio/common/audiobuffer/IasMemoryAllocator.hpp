/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasMemoryAllocator.hpp
 * @date   2015
 * @brief
 */

#ifndef IASMEMORYALLOCATOR_HPP_
#define IASMEMORYALLOCATOR_HPP_

#include <boost/interprocess/managed_heap_memory.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

#include "audio/common/IasAudioCommonTypes.hpp"

namespace IasAudio {

/**
 * @brief Memory allocator
 *
 * This class is used for uniformly allocating memory either in shared or in heap memory
 */
class __attribute__ ((visibility ("default"))) IasMemoryAllocator
{
  public:
    /**
     * @brief Open flags for the shared or heap memory
     */
    enum IasOpenFlag
    {
      eIasCreate,       //!< Create the shared or the heap memory
      eIasConnect       //!< Connect to a previously created shared memory
    };

    /**
     * @brief Constructor
     *
     * @param[in] name The name of the memory object
     * @param totalMemorySize The maximum required memory size
     * @param shared If true allocates the buffer in shared memory, if false allocates the buffer in heap memory
     */
    IasMemoryAllocator(const std::string &name, uint32_t totalMemorySize, bool shared);

    /**
     * Destructor
     */
    ~IasMemoryAllocator();

    /**
     * @brief Initialize the memory allocator
     *
     * @param[in] flag The open flag
     * @return The result of initializing the memory allocator
     * @retval cOk Ok
     * @retval cInvalidParam The heap memory does not allow eIasConnect as open flag
     * @retval cInitFailed The directory in shared memory couldn't be created
     */
    IasAudioCommonResult init(IasOpenFlag flag);

    /**
     * @brief Allocate aligned buffer(s)
     *
     * @param[in] alignment The alignment of the buffer in bytes
     * @param[in] size The size of the buffer in bytes
     * @param[in,out] buffer Pointer to the allocated buffer is returned via this param
     * @return The result of allocating aligned buffer(s)
     * @retval cOk Ok
     * @retval cNotInitialized Memory allocator not initialized
     * @retval cInvalidParam Pointer is NULL
     * @retval cMemoryError Error allocating memory
     */
    IasAudioCommonResult allocate(uint32_t alignment, uint32_t size, void **buffer);

    /**
     * @brief Deallocate buffer
     *
     * @param[in] buffer Pointer to previously allocated buffer
     */
    void deallocate(void *buffer);

    /**
     * @brief Allocate named object(s)
     *
     * @param[in] name Name of the object(s)
     * @param numberItems[in] Number of items to allocate
     * @param array[in,out] Pointer to the object(s) is returned via this param
     * @return The result for allocating the object(s)
     * @retval cOk Ok
     * @retval cNotInitialized Memory allocator not initialized
     */
    template <class T>
    IasAudioCommonResult allocate(const std::string &name, uint32_t numberItems, T **array);

    /**
     * @brief Allocate anonymous object(s)
     *
     * @param numberItems[in] Number of items to allocate
     * @param array[in,out] Pointer to the object(s) is returned via this param
     * @return The result for allocating the object(s)
     * @retval cOk Ok
     * @retval cNotInitialized Memory allocator not initialized
     */
    template <class T>
    IasAudioCommonResult allocate(uint32_t numberItems, T **array);

    /**
     * @brief Deallocate previously allocated object(s) pointed to by ptr
     *
     * @param[in] ptr Pointer to object(s)
     */
    template <class T>
    void deallocate(const T *ptr);

    /**
     * @brief Find previously created object(s)
     * @param[in] name Name of the object(s)
     * @param[in,out] numberItems Number of found objects is returned via this param
     * @param[in,out] array Pointer to found object(s) is returned via this param
     * @return The result of finding the object(s)
     * @retval cOk Ok
     * @retval cNotInitialized Memory allocator not initialized
     * @retval cInvalidParam Pointer is NULL
     * @retval cObjectNotFound Object not found in memory
     */
    template <class T>
    IasAudioCommonResult find(const std::string &name, uint32_t *numberItems, T **array);

    /**
     * @brief Get free memory of full memory block
     * @param[in,out] freeMem Free memory in bytes is returned via this param
     * @return The result of getting the free memory
     * @retval cOk Ok
     * @retval cNotInitialized Memory allocator not initialized
     * @retval cInvalidParam Pointer is NULL
     */
    IasAudioCommonResult getFreeMemory(uint32_t *freeMem) const;

    template <class T>
    IasAudioCommonResult getNumberItems(const T *array, uint32_t *numberItems);

    /**
     * @brief Change the group id of the created shared memory file
     *
     * The group name has to be one of the groups that the calling process is a member of.
     * If the group id does not exist or if the group cannot be changed
     * the method will fail
     *
     * @param[in] groupName The name of the group
     * @param[out] errorMsg If an error occurred, a detailed message will be returned via this parameter
     *
     * @returns The result of changing the group
     * @retval cOk Ok
     * @retval cInvalidParam Failed to change the group
     */
    IasAudioCommonResult changeGroup(const std::string &groupName, std::string *errorMsg);

  private:


    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasMemoryAllocator(IasMemoryAllocator const &other);

    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasMemoryAllocator& operator=(IasMemoryAllocator const &other);

    /**
     * @brief Allocate aligned buffer(s) in shared memory
     * @param[in] alignment The alignment of the buffer in bytes
     * @param[in] size The size of the buffer in bytes
     * @param[in,out] buffer Pointer to the allocated buffer is returned via this param
     * @return The result of allocating aligned buffer(s)
     * @retval cOk Ok
     * @retval cNotInitialized Memory allocator not initialized
     * @retval cInvalidParam Pointer is NULL
     * @retval cMemoryError Error allocating memory
     */
    IasAudioCommonResult allocateShared(uint32_t alignment, uint32_t size, void **buffer);

    /**
     * @brief Deallocate previously allocated buffer(s) in shared memory
     * @param[in] buffer Pointer to previously allocated buffer(s)
     */
    void deallocateShared(void *buffer);

    /**
     * @brief Allocate aligned buffer(s) in heap memory
     * @param[in] alignment The alignment of the buffer in bytes
     * @param[in] size The size of the buffer in bytes
     * @param[in,out] buffer Pointer to the allocated buffer is returned via this param
     * @return The result of allocating aligned buffer(s)
     * @retval cOk Ok
     * @retval cNotInitialized Memory allocator not initialized
     * @retval cInvalidParam Pointer is NULL
     * @retval cMemoryError Error allocating memory
     */
    IasAudioCommonResult allocateHeap(uint32_t alignment, uint32_t size, void **buffer);

    /**
     * @brief Deallocate previously allocated buffer(s) in heap memory
     * @param[in] buffer Pointer to previously allocated buffer(s)
     */
    void deallocateHeap(void *buffer);

    // Member variables
    bool                                   mInitialized;       //!< If true memory allocator is initialized
    std::string                                 mName;              //!< Name of the memory block. Only relevant for shared memory
    uint32_t                                 mTotalMemorySize;   //!< Total memory size of the whole allocated block
    bool                                   mShared;            //!< If true memory is allocated in shared memory otherwise in heap memory
    const std::string                           mFullName;          //!< Fully qualified name in shared memory
    boost::interprocess::managed_shared_memory *mManaged_shm;       //!< Instance is used for shared memory management
    boost::interprocess::managed_heap_memory   *mManaged_heap;      //!< Instance is used for heap memory management
};

template <class T>
IasAudioCommonResult IasMemoryAllocator::allocate(const std::string &name, uint32_t numberItems, T **array)
{
  using namespace boost::interprocess;

  if (mInitialized == false)
  {
    return eIasResultNotInitialized;
  }
  if (mShared == false)
  {
    std::string fullName = mFullName + "_" + name;
    *array = mManaged_heap->construct<T>(fullName.c_str())[numberItems]();
  }
  else
  {
    std::string fullName = mFullName + "_" + name;
    *array = mManaged_shm->construct<T>(fullName.c_str())[numberItems]();
  }
  return eIasResultOk;
}

template <class T>
IasAudioCommonResult IasMemoryAllocator::allocate(uint32_t numberItems, T **array)
{
  using namespace boost::interprocess;

  if (mInitialized == false)
  {
    return eIasResultNotInitialized;
  }
  if (mShared == false)
  {
    *array = mManaged_heap->construct<T>(anonymous_instance)[numberItems]();
  }
  else
  {
    *array = mManaged_shm->construct<T>(anonymous_instance)[numberItems]();
  }
  return eIasResultOk;
}

template <class T>
void IasMemoryAllocator::deallocate(const T *ptr)
{
  using namespace boost::interprocess;

  if (mInitialized == true)
  {
    if (mShared == false)
    {
      mManaged_heap->destroy_ptr(ptr);
    }
    else
    {
      mManaged_shm->destroy_ptr(ptr);
    }
  }
}

template <class T>
IasAudioCommonResult IasMemoryAllocator::find(const std::string &name, uint32_t *numberItems, T **array)
{
  using namespace boost::interprocess;
  if (mInitialized == false)
  {
    return eIasResultNotInitialized;
  }
  if (mShared == false || numberItems == NULL || array == NULL)
  {
    return eIasResultInvalidParam;
  }
  std::string fullName =  mFullName + "_" + name;
  std::pair<T*, managed_shared_memory::size_type> ret = mManaged_shm->find<T>(fullName.c_str());
  T *objectPtr = ret.first;
  if (objectPtr == NULL)
  {
    return eIasResultObjectNotFound;
  }
  *array = objectPtr;
  *numberItems = static_cast<uint32_t>(ret.second);
  return eIasResultOk;
}

template <class T>
IasAudioCommonResult IasMemoryAllocator::getNumberItems(const T *array, uint32_t *numberItems)
{
  if (mInitialized == false)
  {
    return eIasResultNotInitialized;
  }
  if (array == NULL || numberItems == NULL)
  {
    return eIasResultInvalidParam;
  }
  if (mShared == false)
  {
    *numberItems = static_cast<uint32_t>(mManaged_heap->get_instance_length(array));
  }
  else
  {
    *numberItems = static_cast<uint32_t>(mManaged_shm->get_instance_length(array));
  }
  return eIasResultOk;
}



} //namespace IasAudio

#endif /* IASMEMORYALLOCATOR_HPP_ */
