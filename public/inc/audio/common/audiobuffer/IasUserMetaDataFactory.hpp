/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasUserMetaDataFactory.hpp
 * @date   2015
 * @brief
 */

#ifndef IASUSERMETADATAFACTORY_HPP_
#define IASUSERMETADATAFACTORY_HPP_

#include <iostream>
#include "audio/common/IasAudioCommonTypes.hpp"
#include "audio/common/audiobuffer/IasMemoryAllocator.hpp"



namespace IasAudio {

const std::string postFixMetaData = "_md";
const std::string postFixUserData = "_ud";

template <typename T>
class __attribute__ ((visibility ("default"))) IasUserMetaDataFactory
{
  public:
    /**
     * @brief Constructor.
     */
    IasUserMetaDataFactory(IasMemoryAllocator *memAlloc);

    /**
     * @brief Destructor, virtual by default.
     */
    virtual ~IasUserMetaDataFactory();

    /**
     * Create meta data buffers
     *
     * @param[in]     name Name of the meta data array
     * @param[in]     nrBuffers The number of meta data buffers to create
     * @param[in,out] metaData The created buffers are returned via an array ptr
     * @return  The result of creating the buffers
     * @retval cOk Ok
     * @retval cInvalidParam Invalid parameter
     * @retval cMemoryError Could not allocate memory
     */
    IasAudioCommonResult create(const std::string &name, uint32_t nrBuffers, IasMetaData **metaData);

    /**
     * Destroy the previously created meta data buffers
     *
     * @param[in] metaData Pointer to an array containing the meta data buffers
     * @return The result of destroying the buffers
     * @retval cOk Ok
     * @retval cInvalidParam Invalid parameter
     */
    IasAudioCommonResult destroy(IasMetaData *metaData);

    /**
     * Find meta data buffers created by the server side
     *
     * @param[in] name Name of the meta data buffer object(s)
     * @param[in,out] nrBuffers Returns number of buffers in meta data array
     * @param[in,out] metaData Returns pointer to meta data buffer(s)
     */
    IasAudioCommonResult find(const std::string &name, uint32_t *nrBuffers, IasMetaData **metaData) const;

  private:
    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasUserMetaDataFactory(IasUserMetaDataFactory const &other);

    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasUserMetaDataFactory& operator=(IasUserMetaDataFactory const &other);

    // Member variables
    IasMemoryAllocator      *mMemAlloc;   //!< Pointer to a previously created instance of a memory allocator
};

template <typename T>
IasUserMetaDataFactory<T>::IasUserMetaDataFactory(IasMemoryAllocator *memAlloc)
  :mMemAlloc(memAlloc)
{
}

template <typename T>
IasUserMetaDataFactory<T>::~IasUserMetaDataFactory()
{
}

template <typename T>
IasAudioCommonResult IasUserMetaDataFactory<T>::create(const std::string &name, uint32_t nrBuffers, IasMetaData **metaData)
{
  using namespace boost::interprocess;

  if (mMemAlloc == NULL)
  {
    return eIasResultNotInitialized;
  }
  if ((metaData == NULL) || (nrBuffers == 0))
  {
    return eIasResultInvalidParam;
  }
  T *newUserDataStruct = NULL;
  IasAudioCommonResult result = mMemAlloc->allocate<T>(name + postFixUserData, nrBuffers, &newUserDataStruct);
  if (result != eIasResultOk)
  {
    return result;
  }
  result = mMemAlloc->allocate<IasMetaData>(name + postFixMetaData, nrBuffers, metaData);
  if (result != eIasResultOk)
  {
    mMemAlloc->deallocate<T>(newUserDataStruct);
    return result;
  }
  for (uint32_t idx=0; idx<nrBuffers; ++idx)
  {
    IasMetaDataHeader *header = reinterpret_cast<IasMetaDataHeader*>(&newUserDataStruct[idx]);
    if (header->mMagicNumber != IasMetaDataHeader::cMagicNumber)
    {
      mMemAlloc->deallocate<T>(newUserDataStruct);
      mMemAlloc->deallocate<IasMetaData>(*metaData);
      *metaData = NULL;
      return eIasResultInvalidParam;
    }
    header->mSize = sizeof(T);
    (*metaData)[idx].setParams(&newUserDataStruct[idx], idx, nrBuffers-1);
  }
  result = eIasResultOk;
  return result;
}

template <typename T>
IasAudioCommonResult IasUserMetaDataFactory<T>::destroy(IasMetaData *metaData)
{
  if (mMemAlloc == NULL)
  {
    return eIasResultNotInitialized;
  }
  if (metaData == NULL)
  {
    return eIasResultInvalidParam;
  }
  uint32_t firstIdx = metaData[0].getIndex();
  if (firstIdx != 0)
  {
    std::cout << "[" << __func__ << "]" << " Error: First element of vector is invalid. Index is " << firstIdx << std::endl;
    return eIasResultInvalidParam;
  }
  uint32_t numberItems = 0;
  IasAudioCommonResult result = mMemAlloc->getNumberItems<IasMetaData>(metaData, &numberItems);
  if (result != eIasResultOk)
  {
    return result;
  }
  if (numberItems != metaData[0].getMaxIndex()+1)
  {
    std::cout << "[" << __func__ << "]" << " Error: Size of array=" << numberItems << " does not match expected size=" << metaData[0].getMaxIndex()+1 << std::endl;
    return eIasResultInvalidParam;
  }
  // Grab the pointer of the first element. This is the base pointer of the whole memory block allocated
  T *userData = reinterpret_cast<T*>(metaData[0].getAddr());
  // If created via create method above this pointer cannot be NULL
  IAS_ASSERT(userData != NULL);
  mMemAlloc->deallocate<T>(userData);
  mMemAlloc->deallocate<IasMetaData>(metaData);
  return eIasResultOk;
}

template <typename T>
IasAudioCommonResult IasUserMetaDataFactory<T>::find(const std::string &name, uint32_t *nrBuffers, IasMetaData **metaData) const
{
  if (mMemAlloc == NULL)
  {
    return eIasResultNotInitialized;
  }
  if (nrBuffers == NULL || metaData == NULL)
  {
    return eIasResultInvalidParam;
  }
  IasMetaData *tmpMetaData = NULL;
  uint32_t tmpNrBuffersMetaData = 0;
  IasAudioCommonResult result = mMemAlloc->find(name + postFixMetaData, &tmpNrBuffersMetaData, &tmpMetaData);
  if (result != eIasResultOk)
  {
    return result;
  }
  T *tmpUserData = NULL;
  uint32_t tmpNrBuffersUserData = 0;
  result = mMemAlloc->find(name + postFixUserData, &tmpNrBuffersUserData, &tmpUserData);
  if (result != eIasResultOk)
  {
    return result;
  }
  IAS_ASSERT(tmpNrBuffersMetaData == tmpNrBuffersUserData);

  for (uint32_t idx=0; idx<tmpNrBuffersMetaData; ++idx)
  {
    tmpMetaData[idx].setParams(&tmpUserData[idx], idx, tmpNrBuffersMetaData-1);
  }

  *nrBuffers = tmpNrBuffersMetaData;
  *metaData = tmpMetaData;

  return eIasResultOk;
}


} //namespace IasAudio

#endif /* IASUSERMETADATAFACTORY_HPP_ */
