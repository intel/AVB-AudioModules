/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file    IasSrcFarrow.hpp
 * @brief   Sample rate converter based on Farrow's structure.
 * @date    2015
 */

#ifndef IASSRCFARROW_HPP_
#define IASSRCFARROW_HPP_

// disable conversion warnings for tbb
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#include "tbb/tbb.h"
// turn the warnings back on
#pragma GCC diagnostic pop




/*****************************************************************************
 * @brief Class IasSrcFarrow
 *****************************************************************************
 */

namespace IasAudio {

class IasSrcFarrowFirFilter;

class __attribute__ ((visibility ("default"))) IasSrcFarrow
{
  public:

    /**
     * @brief  Result type of the class IasSrcFarrow.
     */
    enum IasResult
    {
      eIasOk,               //!< Ok, Operation successful
      eIasInvalidParam,     //!< Invalid parameter, e.g., out of range or NULL pointer
      eIasInitFailed,       //!< Initialization of the component failed
      eIasNotInitialized,   //!< Component has not been initialized appropriately
      eIasFailed,           //!< other error
    };

    /*!
     * @brief Type definition describing whether the input buffer or output buffer is a ring buffer or a linear buffer.
     *
     * By default, the push-mode sample rate converter uses a ring buffer for storing the
     * processed samples. Equivalently, the pull-mode sample rate converter uses a ring
     * buffer for reading the input samples from. This behavior can be changed by calling
     * the method setBufferMode().
     */
    enum IasBufferMode
    {
      eIasRingBufferMode,   //!< Buffer works in ring buffer mode. This is the default mode.
      eIasLinearBufferMode  //!< Buffer works in linear buffer mode.
    };

    /*!
     * @brief Type definition for the commands that can be stored in the internal concurrent queue.
     */
    enum IasQueuedCommandType
    {
      eIasSetConversionRatio, //!< Command to set the conversion ratio.
      eIasDetunePitch,        //!< Command to detune the pitch.
      eIasReset,              //!< Command to reset the sample rate converter (clear all internal buffers).
      eIasUndefined           //!< Command not defined, used by constructor
    };

    /*!
     *  @brief Struct comprising all parameters that are required for the commands that are
     *         transmitted via the internal concurrent queue.
     */
    struct IasCommandQueueEntry
    {
      // Constructor
      IasCommandQueueEntry()
        :commandId(eIasUndefined)
        ,coeff(nullptr)
        ,fsRatio(1.0)
        ,fsRatioInv(1.0)
        ,filterLength(0u)
        ,numFilters(0u)
        ,inputBlocklen(0u)
        ,outputBlocklen(0u)
      {
      }

      // Member variables that are used for all commands.
      IasQueuedCommandType commandId;  //!< Command Identifier.

      // Member variables that are used for the command eIasSetConversionRatio.
      float  const *coeff;      //!< pointer to the set of impulse responses
      double  fsRatio;           //!< Conversion ratio: inputRate/outputRate
      double  fsRatioInv;        //!< Conversion ratio: outputRate/inputRate
      uint32_t   filterLength;      //!< Length of the impulse responses used for this conversion rate.
      uint32_t   numFilters;        //!< Number of filters used for this conversion rate.

      // Member variables that are used for the command eIasDetunePitch.
      uint32_t   inputBlocklen;     //!< Block length that will be used at the SRC's input.
      uint32_t   outputBlocklen;    //!< Block length that will be used at the SRC's output.
    };

    /*!
     * @brief Constructor.
     */
    IasSrcFarrow();

    /*!
     * @brief Destructor.
     */
    ~IasSrcFarrow();

    /*!
     * @brief Init function.
     *
     * @param[in] maxNumChannels  Maximum number of channels that can be processed.
     */
    IasResult init(uint32_t  maxNumChannels);

    /*!
     * @brief Set the conversion ratio.
     *
     * @param[in] inputRate   Sample rate at the input port.
     * @param[in] outputRate  Sample rate at the output port.
     */
    IasResult setConversionRatio(uint32_t inputRate,
                                 uint32_t outputRate);

    /*!
     * @brief Detune the conversion ratio such that the sample rate converter
     *        generates a constant number of output samples with each call
     *        of the process function.
     *
     * This function shall be called immediately after the function
     * setConversionRatio(), if the sample rate converter is used in a framework
     * that requires a constant block length at the output. One example for such
     * framework is ALSA.
     *
     * This function detunes the conversion ratio such that it is equivalent to
     * the ratio inputBlocklen/outputBlocklen, mich might be different to the
     * accurate conversion ratio inputRate/outputRate.
     *
     * The sample rate converter can be reset into its normal mode by calling
     * the function setConversionRatio() again.
     *
     * @param[in] inputBlocklen   Number of input samples that will be processed wich each block.
     * @param[in] outputBlocklen  Number of output samples that will be requested with each block.
     */
    IasResult detunePitch(uint32_t inputBlocklen,
                          uint32_t outputBlocklen);

    /*!
     * @brief Set the buffer mode to ring buffer or linear buffer.
     *
     * By default, the sample rate converter applies a ring buffer
     *
     * @li for storing the processed samples (if it operates in push-mode)
     * @li for reading the input samples (if it operates in pull-mode).
     *
     * By means of this method, this behavior can be modified. If the
     * mode is set to eIasLinearBufferMode, the respective buffer
     * operates in linear mode. In linear mode, each call of the
     * processPushMode method writes the new samples starting at the
     * beginning of the output buffer. In linear mode, each call of
     * the processPullMode method reads the input samples starting
     * from the beginning of the input buffer.
     *
     * @param[in] bufferMode  Mode to be used, ring buffer (default) or linear.
     */
    void setBufferMode(IasBufferMode bufferMode);


    /*!
     * @brief Get the gain factor that is applied to the output samples.
     *
     * In order to reduce the risk of overmodulation, in particular if the sample
     * rate converter processes fixed point samples, the IasSrcFarrow multiplies
     * the output stream with a gain factor, which is not greater than 1.0.
     * The gain factor is configured in the header file IasSrcFarrowConfig.hpp.
     *
     * This function returns the gain factor that is applied by the sample rate
     * converter.
     *
     * @returns  The gain factor in linear representation (not in dB).
     */
    float getOutputGain();

    /*!
     * @brief Reset function.
     */
    IasResult reset();

    /*!
     * @brief Process the sample rate converter for one block of @a numInputSamples samples
     *        for @a numChannels channels.
     *
     * This function represents the push-mode implementation of the sample rate converter.
     * This means that the caller can specify how many input samples shall be processed with
     * each call of this function. Therefore, the input buffer is a linear buffer.
     *
     * By default, the sample rate converter uses a ring buffer structure for storing the
     * processed samples. The output ring buffer structure is described by the parameters
     * @a outputBuffers, @a writeIndex, @a readIndex, and @a lengthOutputBuffers.
     * The output buffer mode can be changed to linear buffer mode by means of the method
     * setBufferMode().
     *
     * This method can be used for interleaved and deinterleaved buffers.
     *
     * @param[out] outputBuffers       Vector with @a numChannels pointers to the
     *                                 ring buffers for writing the output samples into.
     * @param[in]  inputBuffers        Vector with pointers to the buffers where
     *                                 the input samples can be read from.
     * @param[in]  outputStride        Distance between two consecutive samples of the
     *                                 same channel within the outputBuffers. Use
     *                                 outputStride=1 for de-interleaved output data and
     *                                 outputStride=numChannels for interleaved output data.
     * @param[in]  inputStride         Distance between two consecutive samples of the
     *                                 same channel within the inputBuffers. Use
     *                                 inputStride=1 for de-interleaved input data and
     *                                 inputStride=numChannels for interleaved input data.
     * @param[out] numGeneratedSamples Number of output samples that have been generated.
     * @param[out] numConsumedSamples  Number of input samples that have been consumed.
     *                                 This might be smaller than numInputSamples (the number
     *                                 of input samples that should be processed) if the output
     *                                 buffer provided not enough space for the samples.
     * @param[out] writeIndex          Current write position of the output ring
     *                                 buffer structure. By means of this parameter
     *                                 the sample rate converter declares at which
     *                                 position it will store the next output sample.
     * @param[in]  readIndex           Current read position of the output ring
     *                                 buffer structure. The caller uses this
     *                                 parameter to tell the sample rate converter
     *                                 until which position the samples from the ring
     *                                 buffer have been read out. This parameter is ignored,
     *                                 if the output buffers operate in linear mode.
     * @param[in]  lengthOutputBuffers Length of the output ring buffers.
     * @param[in]  numInputSamples     Number of valid input samples.
     * @param[in]  numChannels         Number of channels that shall be processed.
     * @param[in]  ratioAdjustment     Allows to compensate for the clock skew.
     *                                 For synchronous operation, 1.0 shall be used.
     *                                 The value ratioAdjustment refers to fs1/fs2, which
     *                                 corresponds to inputRate/outputRate for push mode operation.
     */
    template <typename T1, typename T2>
    IasResult processPushMode(T2                 **outputBuffers,
                              T1           const **inputBuffers,
                              uint32_t          outputStride,
                              uint32_t          inputStride,
                              uint32_t         *numGeneratedSamples,
                              uint32_t         *numConsumedSamples,
                              uint32_t         *writeIndex,
                              uint32_t          readIndex,
                              uint32_t          lengthOutputBuffers,
                              uint32_t          numInputSamples,
                              uint32_t          numChannels,
                              float         ratioAdjustment);


    /*!
     * @brief Process the sample rate converter and generate one block of
     *        @a numOutputSamples samples for @a numChannels channels.
     *
     * This function represents the pull-mode implementation of the sample rate converter.
     * This means that the caller can specify how many output samples shall be generated with
     * each call of this function. Therefore, the output buffer is a linear buffer, whereas the
     * input buffer is organized as a ring buffer. The input ring buffer structure is described
     * by the parameters @a inputBuffers, @a writeIndex, @a readIndex, and @a lengthInputBuffers.
     *
     * This method can be used for interleaved and deinterleaved buffers.
     *
     * @param[out] outputBuffers       Vector with @a numChannels pointers to the output
     *                                 buffers for writing the output samples into.
     * @param[in]  inputBuffers        Vector with @a numChannels pointers to the ring buffers
     *                                 where the input samples can be read from.
     * @param[in]  outputStride        Distance between two consecutive samples of the
     *                                 same channel within the outputBuffers. Use
     *                                 outputStride=1 for de-interleaved output data and
     *                                 outputStride=numChannels for interleaved output data.
     * @param[in]  inputStride         Distance between two consecutive samples of the
     *                                 same channel within the inputBuffers. Use
     *                                 inputStride=1 for de-interleaved input data and
     *                                 inputStride=numChannels for interleaved input data.
     * @param[out] numGeneratedSamples Number of output samples that have been generated.
     *                                 This might be smaller than numOutputSamples (the number
     *                                 of output samples that should be generated) if the input
     *                                 buffer provided not enough samples.
     * @param[out] numConsumedSamples  Number of input samples that have been consumed.
     * @param[out] readIndex           Current read position of the input ring buffer
     *                                 structure. By means of this parameter the sample rate
     *                                 converter declares the position from which it will read
     *                                 the next input sample.
     * @param[in]  writeIndex          Current write position of the input ring buffer structure.
     *                                 The caller must use this parameter to tell the sample
     *                                 rate converter until which position the samples in the
     *                                 input ring buffer are valid. This parameter is ignored,
     *                                 if the input buffers operate in linear mode.
     * @param[in]  lengthInputBuffers  Length of the input ring buffers.
     * @param[in]  numOutputSamples    Number of output samples that shall be generated by the
     *                                 sample rate converter.
     * @param[in]  numChannels         Number of channels that shall be processed.
     * @param[in]  ratioAdjustment     Allows to compensate for the clock skew.
     *                                 For synchronous operation, 1.0 shall be used.
     *                                 The value ratioAdjustment refers to fs1/fs2, which
     *                                 corresponds to outputRate/inputRate for pull mode operation.
     */
    template <typename T1, typename T2>
    IasResult processPullMode(T2                 **outputBuffers,
                              T1           const **inputBuffers,
                              uint32_t          outputStride,
                              uint32_t          inputStride,
                              uint32_t         *numGeneratedSamples,
                              uint32_t         *numConsumedSamples,
                              uint32_t         *readIndex,
                              uint32_t          writeIndex,
                              uint32_t          lengthInputBuffers,
                              uint32_t          numOutputSamples,
                              uint32_t          numChannels,
                              float         ratioAdjustment);


  private:
    /*!
     *  @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasSrcFarrow(IasSrcFarrow const &other); //lint !e1704

    /*!
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasSrcFarrow& operator=(IasSrcFarrow const &other); //lint !e1704

    /*!
     *  @brief Pop and execute all commands from the internal queue (private function).
     *
     *  This function pops all commands that are stored in the TBB concurrent queue
     *  and executes them so that the new parameters become effective.
     */
    IasResult executeQueuedCommands();

    /*!
     *  @brief Private function to update the time variant impulse response, using N=4 prototype impulse responses.
     *  @param[out] Buffer for storing the time variant impulse response.
     *              Must be big enough to carry mFilterLength coefficients.
     */
    void updateImpulseResponseN4(float *destination, float tValue) const;

    /*!
     *  @brief Private function to update the time variant impulse response, using N=5 prototype impulse responses.
     *  @param[out] Buffer for storing the time variant impulse response.
     *              Must be big enough to carry mFilterLength coefficients.
     */
    void updateImpulseResponseN5(float *destination, float tValue) const;

    /*!
     *  @brief Private function to update the time variant impulse response, using N=6 prototype impulse responses.
     *  @param[out] Buffer for storing the time variant impulse response.
     *              Must be big enough to carry mFilterLength coefficients.
     */
    void updateImpulseResponseN6(float *destination, float tValue) const;

    /*!
     *  @brief Private function to update the time variant impulse response, using N=7 prototype impulse responses.
     *  @param[out] Buffer for storing the time variant impulse response.
     *              Must be big enough to carry mFilterLength coefficients.
     */
    void updateImpulseResponseN7(float *destination, float tValue) const;


    /*!
     *  @brief Private constants.
     */
    uint32_t static const cMaxFilterLength = 128; //!< maximum length of the impulse responses
    uint32_t static const cMaxNumFilters   = 7;   //!< maximum number of impulse responses

    /*!
     *  @brief Member variables.
     */
    IasBufferMode           mBufferMode;        //!< buffer mode: ring buffer (default) or linear buffer
    double            mTValue;            //!< T value
    float            mTValueHat;         //!< T value for pull mode
    double            mFsRatio;           //!< conversion ratio: fs_in / fs_out
    double            mFsRatioInv;        //!< inverse conversion ratio: fs_out / fs_in for pull mode
    double            mDetuneFactor;      //!< factor for de-tuning the conversion ratio, see IasSrcFarrow::detunePitch
    bool               mDetunedMode;       //!< flag indicating whether conversion ratio shall be de-tuned
    uint32_t             mMaxNumChannels;    //!< maximum number of input channels to be processed in parallel
    uint32_t             mRingBufferIndex;   //!< index for accessing the ring buffer (write index for push-mode, read index for pull-mode)
    uint32_t             mFilterLength;      //!< Length of the impulse responses used for this conversion rate.
    uint32_t             mNumFilters;        //!< Number of filters used for this conversion rate.
    bool               mIsInitialized;     //!< becomes true after IasSrcFarrow::init() has been called

    float const     *mImpulseResponses[cMaxNumFilters]; //!< vector with pointers to the prototype impulse responses
    float           *mTimeVarImpResp;                   //!< pointer to buffer with time-variant impulse response
    IasSrcFarrowFirFilter  *mFirFilterMultiChan;               //!< pointer to the multi-channel FIR filter
    tbb::concurrent_queue<IasCommandQueueEntry> mCommandQueue; //!< internal queue for buffering commands.

    //! Function pointer to address the approprate filter update function (depending on mNumFilters).
    void (IasSrcFarrow::*mUpdateImpulseResponseFunction)(float*, float) const;
};


/**
 * @brief Function to get a IasSrcFarrow::IasResult as string.
 *
 * @return String carrying the result message.
 */
__attribute__ ((visibility ("default"))) std::string toString(const IasSrcFarrow::IasResult& type);


} // namespace IasAudio


#endif // IASSRCFARROW_HPP_
