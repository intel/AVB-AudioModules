/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasMetaDataFactory.cpp
 * @date   2015
 * @brief  This is a wrapper for the IasUserMetaDataFactory template class
 *
 * It has to be provided as source code to allow definition of user-defined data struct
 */

#include "audio/common/audiobuffer/IasMetaDataFactory.hpp"
#include "audio/common/audiobuffer/IasMetaData.hpp"
#include "audio/common/audiobuffer/IasUserMetaDataFactory.hpp"

namespace IasAudio {

struct IasUserMetaData
{
  IasMetaDataHeader header;   //<! Always has to be first member of user-defined meta data
  uint32_t       example1;
  uint32_t       example2;
};

struct IasMetaDataFactory::IasMetaDataFactoryInternal
{
  /**
   * Constructor
   * @param[in] memAlloc Pointer to the memory allocator instance to use
   */
  IasMetaDataFactoryInternal(IasMemoryAllocator *memAlloc)
    :mUserFactory(memAlloc)
  {}

  // Member variables
  IasUserMetaDataFactory<IasUserMetaData>   mUserFactory;
};

IasMetaDataFactory::IasMetaDataFactory(IasMemoryAllocator *memAlloc)
  :mInternal(new IasMetaDataFactoryInternal(memAlloc))
{
  //Nothing to do here
}

IasMetaDataFactory::~IasMetaDataFactory()
{
  delete mInternal;
}

IasAudioCommonResult IasMetaDataFactory::create(const std::string &name, uint32_t nrBuffers, IasMetaData **metaData)
{
  return mInternal->mUserFactory.create(name, nrBuffers, metaData);
}

IasAudioCommonResult IasMetaDataFactory::destroy(IasMetaData *metaData)
{
  return mInternal->mUserFactory.destroy(metaData);
}

IasAudioCommonResult IasMetaDataFactory::find(const std::string &name, uint32_t *nrBuffers, IasMetaData **metaData) const
{
  return mInternal->mUserFactory.find(name, nrBuffers, metaData);
}

uint32_t IasMetaDataFactory::getSize(uint32_t nrBuffers)
{
  return static_cast<uint32_t>(nrBuffers * sizeof(IasUserMetaData) + nrBuffers * sizeof(IasMetaData));
}



} // namespace IasAudio
