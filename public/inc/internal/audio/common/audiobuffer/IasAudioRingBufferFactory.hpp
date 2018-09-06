/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAudioRingBufferFactory.hpp
 * @date   2015
 * @brief
 */

#ifndef IASAUDIORINGBUFFERFACTORY_HPP_

#define IASAUDIORINGBUFFERFACTORY_HPP_


#include "audio/common/IasAudioCommonTypes.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"

namespace IasAudio
{

const uint32_t maxNumRingBufChannels = 8;

class IasAudioRingBuffer;
class IasMemoryAllocator;
class IasAudioSampleBuffer;
class IasMetaData;

using IasMemoryAllocatorMap = std::map<IasAudioRingBuffer*,IasMemoryAllocator*>;

class __attribute__ ((visibility ("default"))) IasAudioRingBufferFactory
{
  public:
    /**
     * @brief the function return the pointer to the factory. If not yet created, the factory will be created
     *
     * @returns pointer to the factory
     */
    static IasAudioRingBufferFactory* getInstance();

    /**
     * @brief function to create and init a ringbuffer
     *
     * @param[in] rinbuffer pointer where the created object is stored
     * @param[in] periodSize period size in samples
     * @param[in] numPeriods the number of periods
     * @param[in] numChannels number of channels per period
     * @param[in] dataFormat the format of the PCM samples (int16, int32, float32)
     * @param[in] dataLayout interleaved layout or de-interleaved
     * @param[in] shared flag to indicate if buffer shall be located in shared memory
     * @param[in] name the name of the shared memory
     *
     * @return cInitFailed FileDescriptor could not be created.
     */
    IasAudioCommonResult createRingBuffer(IasAudioRingBuffer **ringbuffer,
                                          uint32_t periodSize,
                                          uint32_t numPeriods,
                                          uint32_t numChannels,
                                          IasAudioCommonDataFormat dataFormat,
                                          IasRingbufferType form,
                                          std::string name,
                                          std::string groupName = "ias_audio");

    /**
     * @brief the function destroys a ringbuffer
     *
     * @param ringBuf pointer to the buffer that shall be destroyed
     */
    void destroyRingBuffer(IasAudioRingBuffer* ringBuf);

    /**
     * @brief the function is used to find a ringbuffer in shared memory
     *
     * @param[in] name the name of the shared memory
     */
    IasAudioRingBuffer* findRingBuffer(std::string name);

  private:
    /**
     * @brief destructor
     */
    ~IasAudioRingBufferFactory();

    /**
     * @brief constructor
     */
    IasAudioRingBufferFactory();


    /**
     * @brief copy constructor
     */
    IasAudioRingBufferFactory( IasAudioRingBufferFactory const &other );

    /**
     * @brief assign operator
     */
    IasAudioRingBufferFactory& operator=(IasAudioRingBufferFactory const &other);

    IasAudioCommonResult createMirror(uint32_t numChannels);


    IasMemoryAllocatorMap             mMemoryMap; //!< map where the allocated memories and the ringbuffer pointers are stored
    DltContext                       *mLog;       //!< The DLT log context
};


}
#endif
