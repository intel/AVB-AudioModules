/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasMetaDataFactory.hpp
 * @date   2015
 * @brief
 */

#ifndef IASMETADATAFACTORY_HPP_
#define IASMETADATAFACTORY_HPP_


#include "audio/common/IasAudioCommonTypes.hpp"
#include "audio/common/audiobuffer/IasMetaData.hpp"

namespace IasAudio {

class IasMemoryAllocator;

class IasMetaDataFactory
{
  public:
    /**
     * @brief Constructor.
     * @param[in] memAlloc Pointer to memory allocator instance to use
     */
    IasMetaDataFactory(IasMemoryAllocator *memAlloc);

    /**
     * @brief Destructor, virtual by default.
     */
    virtual ~IasMetaDataFactory();

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

    /**
     * Get the size in bytes required to allocate nrBuffers of meta data
     * @param[in] nrBuffers The number of meta data buffers
     * @return The size in bytes required to allocate nrBuffers of meta data
     */
    static uint32_t getSize(uint32_t nrBuffers);

  private:
    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasMetaDataFactory(IasMetaDataFactory const &other);

    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasMetaDataFactory& operator=(IasMetaDataFactory const &other);

    // Member variables
    struct IasMetaDataFactoryInternal;      //<! Forward declaration of private implementation class
    IasMetaDataFactoryInternal *mInternal;  //<! Private implementation class
};

} //namespace IasAudio

#endif /* IASMETADATAFACTORY_HPP_ */
