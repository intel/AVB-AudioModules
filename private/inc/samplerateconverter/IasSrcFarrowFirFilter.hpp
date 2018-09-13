/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file    IasSrcFarrowFirFilter.hpp
 * @brief   FIR filter to be used within the sample rate converter
 *          based on Farrow's structure.
 * @date    2015
 *
 * The FIR filter can be used in two different multi-channel modes:
 *
 * @li One single input channel is filtered by N different impulse
 *     responses. This results in N different output signals.
 *     This is supported by the functions
 *     IasSrcFarrowFirFilter::singleInputInsertSample() and
 *     IasSrcFarrowFirFilter::singleInputProcessSample().
 *
 * @li M individual input channels are filtered by one impulse
 *     responses, which is jointly used for all channels. This
 *     results in M different output signals.
 *     This is supported by the functions
 *     IasSrcFarrowFirFilter::multiInputInsertSample() and
 *     IasSrcFarrowFirFilter::multiInputProcessSample().
 *
 * Depending on the selected mode, the parameters of the function
 * IasSrcFarrowFirFilter::init() have to be set appropriately.
 */

#ifndef IASSRCFARROWFIRFILTER_HPP_
#define IASSRCFARROWFIRFILTER_HPP_


#include "audio/common/IasAudioCommonTypes.hpp"
// Include header file with (branch-specific) configuration parameters.
#include "samplerateconverter/IasSrcFarrowConfig.hpp"

namespace IasAudio {


/*****************************************************************************
 * @brief Class IasSrcFarrowFirFilter
 *****************************************************************************
 */

class IasSrcFarrowFirFilter
{
  public:
    /*!
     * @brief Constructor.
     */
    IasSrcFarrowFirFilter();

    /*!
     * @brief Destructor.
     */
    ~IasSrcFarrowFirFilter();

    /*!
     * @brief Init function.
     *
     * @param[in] maxFilterLength      Maximum length of the FIR filters.
     * @param[in] numImpulseResponses  Number of different impulse responses.
     * @param[in] maxNumInputChannels  Maximum number of input channels.
     */
    int init(uint32_t  maxFilterLength,
             uint32_t  numImpulseResponses,
             uint32_t  maxNumInputChannels);

    /*!
     * @brief Set the filter length
     *
     * @param[in] filterLength Length of the final impulse response.
     *
     * This method has to be called if the multiInputInsertSample method
     * is used before setImpulseResponse is called.
     */
    int setFilterLength(uint32_t filterLength);

    /*!
     * @brief Set the impulse response.
     *
     * @param[in] impulseResponse  Pointer to the vector with pointers to the
     *                             impulse responses that shall be adopted. The
     *                             filter module does not copy the contents of
     *                             the impulse responses; therefore the impulse
     *                             responses must remain constant after this
     *                             fucntion has been called. The length of the
     *                             vector must fit to the parameter
     *                             @a numImpulseResponses, which has been
     *                             used while calling the function
     *                             IasSrcFarrowFirFilter::init().
     * @param[in] filterLength     Length of each impulse response, must not be
     *                             longer than the maximum length that has been
     *                             declared by means of the method
     *                             IasSrcFarrowFirFilter::init().
     */
    int setImpulseResponse(float const **impulseResponses,
                           uint32_t          filterLength);

    /*!
     * @brief Reset function.
     */
    int reset();

    /*!
     * @brief Insert a new input sample into the internal ring buffer.
     *
     * This function shall be used together with
     * IasSrcFarrowFirFilter::singleInputProcessSample(), which processes the
     * convolution of the input signal with N different impulse responses.
     * This results in N different output signals.
     *
     * @param[in]  inputBuffers     Pointer to the input buffer.
     * @param[in]  inputBufferIndex Index defining what sample inside @a inputBuffer
     *                              is the current sample.
     */
    int singleInputInsertSample(float const *inputBuffer,
                                uint32_t         inputBufferIndex);

    /*!
     * @brief Process the FIR filter for one sample: this function filters
     *        one single input channel with N different impulse responses.
     *        This results in N different output signals.
     *
     * This function shall be used together with
     * IasSrcFarrowFirFilter::singleInputInsertSample(), which inserts a new
     * input sample into the internal ring buffer.
     *
     * @param[out] outputSamples     Vector for writing the output samples into.
     *                               The vector must provide space for N output
     *                               samples, according to the with N different
     *                               inpulse responses.
     */
    int singleInputProcessSample(float *outputSamples);

    /*!
     * @brief Insert a new input sample (of M input channels) into the internal
     *        ring buffers.
     *
     * This function shall be used together with
     * IasSrcFarrowFirFilter::multiInputProcessSample(), which processes the
     * convolution of the N input signals with one common impulse response,
     * which is jointly used for all channels. This results in M different
     * output signals.
     *
     * @param[in]  inputBuffers     Vector with pointers to the M input buffers.
     * @param[in]  inputBufferIndex Index defining what sample inside @a inputBuffers
     *                              is the current sample.
     * @param[in]  numInputChannels Actual number of input channels (might be smaller
     *                              than @a maxNumInputChannels, as defined during
     *                              the initialization of the module).
     */
    template <typename T>
    int multiInputInsertSample(T            const **inputBuffers,
                               uint32_t          inputBufferIndex,
                               uint32_t          numInputChannels);

    /*!
     * @brief Process the FIR filter for one sample: this function filters
     *        M individual input channels by one single impulse response,
     *        which is jointly used for all channels. This results in
     *        M different output signals.
     *
     * This function shall be used together with
     * IasSrcFarrowFirFilter::multiInputInsertSample(), which inserts a new
     * input sample (of M input channels) into the internal ring buffers.
     *
     * For this function, multiple versions (with and without SSE optimization)
     * have been implemented. The flag IASSRCFARROWCONFIG_USE_SSE controls
     * which implementation is used.
     *
     * @param[out] outputBuffers     Vector with pointers to the M output buffers.
     *                               The current output sample of the M parallel
     *                               will be written at the position that is
     *                               specified by @a outputBufferIndex.
     * @param[in]  outputBufferIndex Defines at which position (inside outputBuffers)
     *                               the current output samples shall be written to.
     * @param[in]  numChannels       Actual number of channels (might be smaller
     *                               than @a maxNumInputChannels, as defined during
     *                               the initialization of the module).
     */
    template <typename T>
    int multiInputProcessSample(T            **outputBuffers,
                                uint32_t    outputBufferIndex,
                                uint32_t    numChannels);

#if IASSRCFARROWCONFIG_USE_SSE
    inline uint32_t getPaddingForSSE() { return mPaddingForSSE; }
#endif // #if IASSRCFARROWCONFIG_USE_SSE

  private:
    /*!
     *  @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasSrcFarrowFirFilter(IasSrcFarrowFirFilter const &other); //lint !e1704

    /*!
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasSrcFarrowFirFilter& operator=(IasSrcFarrowFirFilter const &other); //lint !e1704


    /*!
     *  Member variables.
     */
    float const **mImpulseResponses;    //!< vector with pointers to the impulse responses
    float       **mRingBuffers;         //!< vector with pointers to the ring buffers, each ring buffer
    //!< carries the most recent input samples (two copies)
    uint32_t          mRingBufferIndex;     //!< the next input sample will be stored at this position

#if IASSRCFARROWCONFIG_USE_SSE
    uint32_t          mPaddingForSSE;       //!< offset for SSE
#endif

    uint32_t          mMaxFilterLength;     //!< maximum length of (each) impulse response
    uint32_t          mFilterLength;        //!< actual length of (each) impulse response
    uint32_t          mNumImpulseResponses; //!< number of impulse responses to be applied in parallel
    uint32_t          mMaxNumInputChannels; //!< maximum number of input channels to be processed in parallel
    bool            mIsInitialized;       //!< becomes true after IasSrcFarrowFirFilter::init() has been called
};

} // namespace IasAudio

#endif // IASSRCFARROWFIRFILTER_HPP_
