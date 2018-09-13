/*
 * @COPYRIGHT_TAG@
 */
/*
 * IasSrcWrapperBase.hpp
 *
 *  Created 2016
 *
 */

#ifndef IASSRCWRAPPERBASE_HPP_
#define IASSRCWRAPPERBASE_HPP_


#include "audio/common/IasAudioCommonTypes.hpp"

namespace IasAudio{


class IasAudioLogging;

/**
 * @brief The result values
 */
enum IasSrcWrapperResult{
        eIasOk = 0, //!< no error occurred
        eIasFailed  //!< operation failed
      };

/**
 * @brief The init paramter structure
 */
struct IasSrcWrapperParams
{

  IasSrcWrapperParams()
    :inputFormat(eIasFormatUndef)
    ,outputFormat(eIasFormatUndef)
    ,inputSampleRate(0)
    ,outputSampleRate(0)
    ,numChannels(0)
    ,inputIndex(0)
    ,outputIndex(0)
  {}

  IasSrcWrapperParams(IasAudioCommonDataFormat inFormat,
                      IasAudioCommonDataFormat outFormat,
                      uint32_t inRate,
                      uint32_t outRate,
                      uint32_t nChannels,
                      uint32_t inputIdx,
                      uint32_t outputIdx)

    :inputFormat(inFormat)
    ,outputFormat(outFormat)
    ,inputSampleRate(inRate)
    ,outputSampleRate(outRate)
    ,numChannels(nChannels)
    ,inputIndex(inputIdx)
    ,outputIndex(outputIdx)
  {}

  IasAudioCommonDataFormat inputFormat;   //!< input sample format
  IasAudioCommonDataFormat outputFormat;  //!< output sample format
  uint32_t inputSampleRate;            //!< input sample rate
  uint32_t outputSampleRate;           //!< output sample rate
  uint32_t numChannels;                //!< number of channels
  uint32_t inputIndex;                 //!< index of first input channel
  uint32_t outputIndex;                //!< index of first output channel

};

class __attribute__ ((visibility ("default"))) IasSrcWrapperBase
{
  public:

    virtual ~IasSrcWrapperBase(){};

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
                                     const IasAudioArea* outArea) = 0;

    /**
     * @brief Process function of sample rate converter wrapper
     *
     * @params[out] numGeneratedSamples Number of generated samples by the sample rate converter
     * @params[out] numConsumedSamples Number of consumed samples by the sample rate converter
     * @params[in]  lengthInputBuffers Number of samples provided to sample rate converter
     * @params[in]  numOutputSamples The number of sample that still needs to be processed
     * @param[in]   srcOffset The offset in the source buffer
     * @param[in]   sinkOffset The offset in the sink buffer
     *
     * @returns error code
     * @retval eIasOk all went well
     * @retval eIasFailed an error occurred
     */
    virtual IasSrcWrapperResult process(uint32_t *numGeneratedSamples,
                                        uint32_t *numConsumedSamples,
                                        uint32_t lengthInputBuffers,
                                        uint32_t numOutputSamples,
                                        uint32_t srcOffset,
                                        uint32_t sinkOffset) = 0;

    /**
     * @brief Reset function of sample rate converter wrapper
     *
     * @returns error code
     * @retval eIasOk all went well
     * @retval eIasFailed an error occurred
     */
    virtual IasSrcWrapperResult reset() = 0;

};

}

#endif
