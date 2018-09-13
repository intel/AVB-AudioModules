/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file    IasSrcWrapper.hpp
 * @brief   Wrapper class for sample rate converter based on Farrow's structure.
 * @date    2016
 */

#ifndef IASSRCWRAPPER_HPP_
#define IASSRCWRAPPER_HPP_

#include "internal/audio/common/samplerateconverter/IasSrcWrapperBase.hpp"

#include "internal/audio/common/IasAudioLogging.hpp"
#include "audio/common/IasAudioCommonTypes.hpp"
#include "internal/audio/common/samplerateconverter/IasSrcFarrow.hpp"
#include <vector>

namespace IasAudio
{

template <class T1, class T2>
class __attribute__ ((visibility ("default"))) IasSrcWrapper : public IasSrcWrapperBase
{

  public:

    /**
     * @brief The maximum allowed sample rate
     */
    const uint32_t cMaxSampleRate = 96000;

    /**
     * @brief The minimum allowed sample rate
     */
    const uint32_t cMinSampleRate = 8000;


    IasSrcWrapper();

    ~IasSrcWrapper();

    /**
     * @brief Init function of sample rate converter wrapper
     *
     * @params[in] params the init parameter structure
     * @params[in] inArea pointer to the input area
     * @params[in] outArea pointer to the output area
     *
     * @returns error code
     * @retval eIasOk all went well
     * @retval eIasFailed an error occurred
     */
    virtual IasSrcWrapperResult init(IasSrcWrapperParams &params,
                                     const IasAudioArea* inArea,
                                     const IasAudioArea* outArea);

    /**
     * @brief Process function of sample rate converter wrapper
     *
     * @params[out] numGeneratedSamples Number of generated samples by the sample rate converter.
     *                                  For pull mode, this should be constant for every process call
     *                                  as long as there are enough input samples available
     * @params[out] numConsumedSamples  Number of consumed samples by the sample rate converter. For pull mode,
     *                                  this number can vary from call to call depending on the ratio.
     * @params[in]  numInputSamples     Number of available input samples. In pull mode, if there are not enough, the number
     *                                  of generated samples will be smaller than desired.
     * @params[in]  numOutputSamples    For pull mode, this is the desired number of generated output samples
     * @param[in]   srcOffset           The position in the inArea, where the data samples will be taken from
     * @param[in]   sinkOffset          The position in the outArea, where the data samples will be written to
     *
     * @returns error code
     * @retval eIasOk all went well
     * @retval eIasFailed an error occurred
     */
    virtual IasSrcWrapperResult process(uint32_t *numGeneratedSamples,
                                        uint32_t *numConsumedSamples,
                                        uint32_t numInputSamples,
                                        uint32_t numOutputSamples,
                                        uint32_t srcOffset,
                                        uint32_t sinkOffset);

    /**
     * @brief Reset function of sample rate converter wrapper
     *
     * @returns error code
     * @retval eIasOk all went well
     * @retval eIasFailed an error occurred
     */
    virtual IasSrcWrapperResult reset();

  private:
    /*!
     *  @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasSrcWrapper(IasSrcWrapper const &other);

    /*!
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasSrcWrapper& operator=(IasSrcWrapper const &other);

    /**
      * @brief Function to check paramters for errors
      *
      * @param[in] params The init parameter structure
      *
      * @returns error code
      * @retval eIasOk all went well
      * @retval eIasFailed an error occurred
      */
    IasSrcWrapperResult checkParams(IasSrcWrapperParams *params) const;

    DltContext*              mLog;                    //!< The log object
    const IasAudioArea*      mInArea;                 //!< pointer to input IasAudioArea
    const IasAudioArea*      mOutArea;                //!< pointer to output IasAudioArea
    IasAudioCommonDataFormat mInputFormat;            //!< input data format
    IasAudioCommonDataFormat mOutputFormat;           //!< output data format
    uint32_t              mInputSampleRate;        //!< input data sample rate
    uint32_t              mOutputSampleRate;       //!< output data sample rate
    uint32_t              mNumChannels;            //!< the number of channels
    uint32_t              mInputIndex;             //!< start index of first input channel in inputArea
    uint32_t              mOutputIndex;            //!< start index of first output channel in outputArea
    uint32_t              mInputStride;            //!< step size to get to next sample of same input channel
    uint32_t              mOutputStride;           //!< step size to get to next sample of same output channel
    IasSrcFarrow*            mSrc;                    //!< the sample rate converter object
    const T1**               mSrcInputBuffers;        //!< the vector with input buffer pointers used by sample rate converter
    T2**                     mSrcOutputBuffers;       //!< the vector with output buffer pointers used by sample rate converter
    const T1**               mSrcInputBuffersStatic;  //!< the vector with static part of input buffer pointers used by sample rate converter
    T2**                     mSrcOutputBuffersStatic; //!< the vector with static part of output buffer pointers used by sample rate converter
};

}
#endif
