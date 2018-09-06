/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/*
 * IasRingBufferTestReader.hpp
 *
 *  Created 2015
 */

#ifndef IASRINGBUFFERTESTREADER_HPP_
#define IASRINGBUFFERTESTREADER_HPP_

#include "internal/audio/common/audiobuffer/IasAudioRingBuffer.hpp"

#include <sndfile.h>



namespace IasAudio
{
class IasAudioRingBuffer;

class __attribute__ ((visibility ("default"))) IasRingBufferTestReader
{
  public:

    IasRingBufferTestReader(IasAudioRingBuffer* buffer,
                            uint32_t periodSize,
                            uint32_t numChannels,
                            IasAudioCommonDataFormat dataType,
                            std::string fileName);

    ~IasRingBufferTestReader();

    IasAudioCommonResult init();

    IasAudioCommonResult readFromBuffer(uint32_t chanIdx);


  private:

    IasRingBufferTestReader(IasRingBufferTestReader const &other);

    IasRingBufferTestReader& operator=(IasRingBufferTestReader const &other);

    template<typename T>
    IasAudioCommonResult copy( uint32_t numSamples, uint32_t chanIdx);


    IasAudioCommonResult write(sf_count_t* numWrittenSamples,int32_t* src);

    IasAudioCommonResult write(sf_count_t* numWrittenSamples,int16_t* src);

    IasAudioCommonResult write(sf_count_t* numWrittenSamples,float* src);

    IasAudioRingBuffer      *mRingBuffer;
    uint32_t              mPeriodSize;
    uint32_t              mNumChannels;
    int32_t               mSampleSize;
    IasAudioCommonDataFormat mDataType;
    void*                    mReadBuf;
    SNDFILE*                 mDataFile;
    IasAudioArea*            mAreas;
    uint32_t              mNumBufferChannels;
    std::string              mFileName;
    bool                mInitialized;
    uint64_t              mSamplesWrittenToFile;


};

}
#endif
