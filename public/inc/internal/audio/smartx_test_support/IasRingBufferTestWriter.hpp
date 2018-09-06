/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/*
 * IasRingBufferTestWriter.hpp
 *
 *  Created 2015
 */

#ifndef IASRINGBUFFERTESTWRITER_HPP_
#define IASRINGBUFFERTESTWRITER_HPP_


#include "audio/common/IasAudioCommonTypes.hpp"

#include <sndfile.h>

namespace IasAudio
{

class IasAudioRingBuffer;

class __attribute__ ((visibility ("default"))) IasRingBufferTestWriter
{

  public:

    IasRingBufferTestWriter(IasAudioRingBuffer* buffer,
                            uint32_t periodSize,
                            uint32_t numChannels,
                            IasAudioCommonDataFormat dataType,
                            std::string fileName);


    ~IasRingBufferTestWriter();

    IasAudioCommonResult init();

    IasAudioCommonResult writeToBuffer(uint32_t chanIdx);


  private:

    IasRingBufferTestWriter(IasRingBufferTestWriter const &other);

    IasRingBufferTestWriter& operator=(IasRingBufferTestWriter const &other);

    IasAudioCommonResult read( uint32_t* numReadSamples, int16_t* readDest);

    IasAudioCommonResult read( uint32_t* numReadSamples, int32_t* readDest);

    IasAudioCommonResult read( uint32_t* numReadSamples, float* readDest);

    template< typename T>
    IasAudioCommonResult copy( uint32_t numSamples, uint32_t chanIdx);

    IasAudioRingBuffer*      mRingBuffer;
    uint32_t              mPeriodSize;
    uint32_t              mNumChannels;
    int32_t               mSampleSize;
    IasAudioCommonDataFormat mDataType;
    void*                    mWriteBuf;
    SNDFILE*                 mDataFile;
    IasAudioArea*            mAreas;
    uint32_t              mNumBufferChannels;
    std::string              mFileName;
    bool                mInitialized;
    uint64_t              mSamplesReadFromFile;

};

}
#endif
