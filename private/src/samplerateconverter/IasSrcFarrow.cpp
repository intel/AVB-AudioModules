/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file    IasSrcFarrow.cpp
 * @brief   Sample rate converter based on Farrow's structure.
 * @date    2015
 */

#include <cmath>                                  // declare fabsf()
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdlib.h>

#include "audio/common/IasAudioCommonTypes.hpp"
#include "internal/audio/common/samplerateconverter/IasSrcFarrow.hpp"
#include "samplerateconverter/IasSrcFarrowFirFilter.hpp"


#if IASSRCFARROWCONFIG_USE_SSE
#include <xmmintrin.h>
#include <emmintrin.h>
#endif

#ifdef __linux__
#define MS_VC  0
#define ALIGN16 __attribute__((aligned(16)))
#else
#define MS_VC  1
#define ALIGN16 __declspec(align(16))
#endif

#ifdef __INTEL_COMPILER
#define INTEL_COMPILER 1
#else
#define INTEL_COMPILER 0
#endif

#if !(MS_VC)
#include <malloc.h>
#endif

namespace IasAudio {

#define CONVERT_COEFF(x)    (static_cast<const float>(x))

/*!
 *  @brief Struct for defining the filter parameters (filterLength, numFilters).
 *         The filter parameters are specific for each conversion ratio.
 */
typedef struct {
  uint32_t filterLength;  //!< Length of the impulse responses.
  uint32_t numFilters;    //!< Number of impulse responses.
} IasSrcFarrowFilterParams;

/*!
 *  @brief Enum type definition for all supported conversion ratios.
 */
typedef enum {
  eConversionRatio_24000to08000,
  eConversionRatio_24000to16000,
  eConversionRatio_44100to16000,
  eConversionRatio_48000to08000,
  eConversionRatio_48000to11025,
  eConversionRatio_48000to12000,
  eConversionRatio_48000to16000,
  eConversionRatio_48000to22050,
  eConversionRatio_48000to24000,
  eConversionRatio_48000to32000,
  eConversionRatio_48000to44100,
  eConversionRatio_48000to48000,
  eConversionRatio_44100to48000, //!< used also for all upsampling use cases
  eConversionRatioNumValues      //!< number of elements in this enum
} IasConversionRatio;


/*!
 *  @brief Table defining the filter parameters (filterLength, numFilters)
 *         for each conversion ratio.
 */
IasSrcFarrowFilterParams const cFilterParams[eConversionRatioNumValues] =
{
  {
#include "./coeffSrcFarrow/IasSrcFarrow_param_24000Hz_to_08000Hz.h"
  },
  {
#include "./coeffSrcFarrow/IasSrcFarrow_param_24000Hz_to_16000Hz.h"
  },
  {
#include "./coeffSrcFarrow/IasSrcFarrow_param_44100Hz_to_16000Hz.h"
  },
  {
#include "./coeffSrcFarrow/IasSrcFarrow_param_48000Hz_to_08000Hz.h"
  },
  {
#include "./coeffSrcFarrow/IasSrcFarrow_param_48000Hz_to_11025Hz.h"
  },
  {
#include "./coeffSrcFarrow/IasSrcFarrow_param_48000Hz_to_12000Hz.h"
  },
  {
#include "./coeffSrcFarrow/IasSrcFarrow_param_48000Hz_to_16000Hz.h"
  },
  {
#include "./coeffSrcFarrow/IasSrcFarrow_param_48000Hz_to_22050Hz.h"
  },
  {
#include "./coeffSrcFarrow/IasSrcFarrow_param_48000Hz_to_24000Hz.h"
  },
  {
#include "./coeffSrcFarrow/IasSrcFarrow_param_48000Hz_to_32000Hz.h"
  },
  {
#include "./coeffSrcFarrow/IasSrcFarrow_param_48000Hz_to_44100Hz.h"
  },
  {
#include "./coeffSrcFarrow/IasSrcFarrow_param_48000Hz_to_48000Hz.h"
  },
  {
#include "./coeffSrcFarrow/IasSrcFarrow_param_44100Hz_to_48000Hz.h" // used also for all upsampling use cases
  },
};


/*!
 *  @brief Define the filter coefficients for each conversion ratio.
 *
 *  All impulse responses need to be 16-bytes aligned, because otherwise movaps() will crash.
 */
ALIGN16 float static const coeff24000to08000[] = {
#include "./coeffSrcFarrow/IasSrcFarrow_coeff_24000Hz_to_08000Hz.h"
};
ALIGN16 float static const coeff24000to16000[] = {
#include "./coeffSrcFarrow/IasSrcFarrow_coeff_24000Hz_to_16000Hz.h"
};
ALIGN16 float static const coeff44100to16000[] = {
#include "./coeffSrcFarrow/IasSrcFarrow_coeff_44100Hz_to_16000Hz.h"
};
ALIGN16 float static const coeff48000to08000[] = {
#include "./coeffSrcFarrow/IasSrcFarrow_coeff_48000Hz_to_08000Hz.h"
};
ALIGN16 float static const coeff48000to11025[] = {
#include "./coeffSrcFarrow/IasSrcFarrow_coeff_48000Hz_to_11025Hz.h"
};
ALIGN16 float static const coeff48000to12000[] = {
#include "./coeffSrcFarrow/IasSrcFarrow_coeff_48000Hz_to_12000Hz.h"
};
ALIGN16 float static const coeff48000to16000[] = {
#include "./coeffSrcFarrow/IasSrcFarrow_coeff_48000Hz_to_16000Hz.h"
};
ALIGN16 float static const coeff48000to22050[] = {
#include "./coeffSrcFarrow/IasSrcFarrow_coeff_48000Hz_to_22050Hz.h"
};
ALIGN16 float static const coeff48000to24000[] = {
#include "./coeffSrcFarrow/IasSrcFarrow_coeff_48000Hz_to_24000Hz.h"
};
ALIGN16 float static const coeff48000to32000[] = {
#include "./coeffSrcFarrow/IasSrcFarrow_coeff_48000Hz_to_32000Hz.h"
};
ALIGN16 float static const coeff48000to44100[] = {
#include "./coeffSrcFarrow/IasSrcFarrow_coeff_48000Hz_to_44100Hz.h"
};
ALIGN16 float static const coeff48000to48000[] = {
#include "./coeffSrcFarrow/IasSrcFarrow_coeff_48000Hz_to_48000Hz.h"
};
ALIGN16 float static const coeff44100to48000[] = {
#include "./coeffSrcFarrow/IasSrcFarrow_coeff_44100Hz_to_48000Hz.h" // used also for all upsampling use cases
};



/**
 * @brief Increment the buffer index using modulo-length addressing, i.e. the
 *        buffer index is reset to zero if it reaches the end of the buffer.
 *
 * @returns                  The incremented buffer index.
 * @param[in]  currentIndex  The original buffer index.
 * @param[in]  bufferLength  The length of the buffer.
 */
inline uint32_t incrementBufferIndex(uint32_t currentIndex, uint32_t bufferLength)
{
  currentIndex++;
  if (currentIndex >= bufferLength)
  {
    currentIndex = 0;
  }
  return currentIndex;
}


/**
 * @brief Calculate the number of free samples within the ring buffer.
 *
 * @returns                  The number of free samples.
 * @param[in]  readIndex     The index of the latest sample that has been read out.
 * @param[in]  writeIndex    The index of the next sample that will be written.
 * @param[in]  bufferLength  The length of the buffer.
 */
inline uint32_t numFreeSamplesRingbuffer(uint32_t readIndex, uint32_t writeIndex, uint32_t bufferLength)
{
  uint32_t numFreeSamples;

  if (readIndex > writeIndex)
  {
    numFreeSamples = readIndex - writeIndex;
  }
  else
  {
    // This includes the case that (readIndex == writeIndex), which means (by definition)
    // that the buffer is completely free.
    numFreeSamples = bufferLength + readIndex - writeIndex;
  }

  // To avoid that buffer is filled completely, we decrease the number of free
  // samples by one. By means of this we avoid situations where
  // (readIndex == writeIndex) indicate a completely filled buffer.
  numFreeSamples--;

  return numFreeSamples;
}


/**
 * @brief Calculate the number of filled samples within the ring buffer.
 *
 * @returns                  The number of filled samples.
 * @param[in]  writeIndex    The index of the next sample that will be written.
 * @param[in]  readIndex     The index of the latest sample that has been read out.
 * @param[in]  bufferLength  The length of the buffer.
 */
inline uint32_t numFilledSamplesRingbuffer(uint32_t writeIndex, uint32_t readIndex, uint32_t bufferLength)
{
  uint32_t numFilledSamples;

  if (writeIndex >= readIndex)
  {
    // This includes the case that (writeIndex == readIndex), which means (by definition)
    // that the buffer is completely free (i.e. numFilledSamples is 0 then).
    numFilledSamples = writeIndex - readIndex;
  }
  else
  {
    numFilledSamples = bufferLength + writeIndex - readIndex;
  }

  return numFilledSamples;
}


/*****************************************************************************
 * @brief Constructor
 *****************************************************************************
 */
IasSrcFarrow::IasSrcFarrow()
  :mBufferMode(eIasRingBufferMode)
  ,mTValue(0.0)
  ,mFsRatio(1.0)
  ,mDetuneFactor(1.0)
  ,mDetunedMode(false)
  ,mMaxNumChannels(0)
  ,mRingBufferIndex(0)
  ,mFilterLength(0)
  ,mNumFilters(0)
  ,mIsInitialized(false)
  ,mTimeVarImpResp(NULL)
  ,mFirFilterMultiChan(NULL)
  ,mUpdateImpulseResponseFunction(NULL)
{
}


/*****************************************************************************
 * @brief Destructor
 *****************************************************************************
 */
IasSrcFarrow::~IasSrcFarrow()
{
  if (mIsInitialized)
  {
    // Delete the multi-channel FIR filter object required for the alternative approach
    delete mFirFilterMultiChan;
    mFirFilterMultiChan = NULL;

#if MS_VC
    _aligned_free(mTimeVarImpResp);
#else
    free(mTimeVarImpResp);
#endif
    mTimeVarImpResp = NULL;
  }
}


/*****************************************************************************
 * @brief Init function
 *****************************************************************************
 */
IasSrcFarrow::IasResult IasSrcFarrow::init(uint32_t  maxNumChannels)
{
  if (mIsInitialized)
  {
    return eIasInitFailed;
  }

  // Allocate the buffer with the time-variant impulse response.
#if MS_VC
#if IASSRCFARROWCONFIG_USE_SSE
  mTimeVarImpResp = (float*) _aligned_malloc((cMaxFilterLength+4)*sizeof(float), 16);
#else
  mTimeVarImpResp = (float*) _aligned_malloc(cMaxFilterLength*sizeof(float), 16);
#endif // #if IASSRCFARROWCONFIG_USE_SSE
#else
#if IASSRCFARROWCONFIG_USE_SSE
  mTimeVarImpResp = (float*) memalign(16, (cMaxFilterLength+4)*sizeof(float));
#else
  mTimeVarImpResp = (float*) memalign(16, cMaxFilterLength*sizeof(float));
#endif
#endif // #if MS_VC

  if (mTimeVarImpResp == NULL)
  {
    return eIasInitFailed;
  }

  // Create the multi-channel FIR filter object.
  mFirFilterMultiChan = new IasSrcFarrowFirFilter();
  if (mFirFilterMultiChan == NULL)
  {
    return eIasInitFailed;
  }

  mFirFilterMultiChan->init(cMaxFilterLength, 1, maxNumChannels);

  mMaxNumChannels = maxNumChannels;
  mIsInitialized  = true;

  // Call the reset function
  IasResult result = this->reset();
  IAS_ASSERT(result == eIasOk);
  (void)result;

  return eIasOk;
}

/*****************************************************************************
 * @brief Set the conversion ratio.
 *****************************************************************************
 */
IasSrcFarrow::IasResult IasSrcFarrow::setConversionRatio(uint32_t inputRate,
                                                         uint32_t outputRate)
{
  float const *coeff = NULL;
  IasSrcFarrowFilterParams filterParams;

  if ((inputRate == 0) || (outputRate == 0))
  {
    // Avoid possible divisions by zero.
    return eIasInvalidParam;
  }
  else if ((inputRate == 48000) && (outputRate >= 48000))
  {
    // For conversion from 48 kHz to 48 kHz (and above) we can apply shorter filters (M=48),
    // due to the soft slope (transition bandwidth from 20 kHz to 24 kHz).
    coeff = coeff48000to48000;
    filterParams = cFilterParams[eConversionRatio_48000to48000];
  }
  else if (inputRate <= outputRate)
  {
    // All upsampling use cases share the same set of coefficients.
    coeff = coeff44100to48000;
    filterParams = cFilterParams[eConversionRatio_44100to48000];
  }
  else if (inputRate == 48000)
  {
    switch (outputRate) {
      case  8000:
        coeff = coeff48000to08000; filterParams = cFilterParams[eConversionRatio_48000to08000];
        break;
      case 11025:
        coeff = coeff48000to11025; filterParams = cFilterParams[eConversionRatio_48000to11025];
        break;
      case 12000:
        coeff = coeff48000to12000; filterParams = cFilterParams[eConversionRatio_48000to12000];
        break;
      case 16000:
        coeff = coeff48000to16000; filterParams = cFilterParams[eConversionRatio_48000to16000];
        break;
      case 22050:
        coeff = coeff48000to22050; filterParams = cFilterParams[eConversionRatio_48000to22050];
        break;
      case 24000:
        coeff = coeff48000to24000; filterParams = cFilterParams[eConversionRatio_48000to24000];
        break;
      case 32000:
        coeff = coeff48000to32000; filterParams = cFilterParams[eConversionRatio_48000to32000];
        break;
      case 44100:
        coeff = coeff48000to44100; filterParams = cFilterParams[eConversionRatio_48000to44100];
        break;
      default:
        coeff = NULL; filterParams.filterLength = 0; filterParams.numFilters = 0;
        break;
    }
  }
  else if (inputRate == 44100)
  {
    switch (outputRate) {
      case 16000:
        coeff = coeff44100to16000; filterParams = cFilterParams[eConversionRatio_44100to16000];
        break;
      default:
        coeff = NULL; filterParams.filterLength = 0; filterParams.numFilters = 0;
        break;
    }
  }
  else if (inputRate == 24000)
  {
    switch (outputRate) {
      case  8000:
        coeff = coeff24000to08000; filterParams = cFilterParams[eConversionRatio_24000to08000];
        break;
      case 16000:
        coeff = coeff24000to16000; filterParams = cFilterParams[eConversionRatio_24000to16000];
        break;
      default:
        coeff = NULL; filterParams.filterLength = 0; filterParams.numFilters = 0;
        break;
    }
  }

  if (coeff == NULL)
  {
    return eIasInvalidParam;
  }

  IAS_ASSERT(filterParams.filterLength <= cMaxFilterLength);
  IAS_ASSERT(filterParams.numFilters   <= cMaxNumFilters);

  IasCommandQueueEntry queuedCommand;
  queuedCommand.commandId    = eIasSetConversionRatio;
  queuedCommand.fsRatio      = (static_cast<double>(inputRate) /
                                static_cast<double>(outputRate));
  queuedCommand.fsRatioInv   = (static_cast<double>(outputRate) /
                                static_cast<double>(inputRate));
  queuedCommand.coeff        = coeff;
  queuedCommand.filterLength = filterParams.filterLength;
  queuedCommand.numFilters   = filterParams.numFilters;
  mCommandQueue.push(queuedCommand);

  return eIasOk;
}

/*****************************************************************************
 * @brief Detune the conversion ratio.
 *****************************************************************************
 */
IasSrcFarrow::IasResult IasSrcFarrow::detunePitch(uint32_t inputBlocklen,
                                                  uint32_t outputBlocklen)
{
  // Avoid that a division by zero occurs in the course of the methods
  // IasSrcFarrow::executeQueuedCommands or IasSrcFarrow::processPullMode.
  if ((inputBlocklen == 0) || (outputBlocklen == 0))
  {
    return eIasInvalidParam;
  }

  IasCommandQueueEntry queuedCommand;
  queuedCommand.commandId      = eIasDetunePitch;
  queuedCommand.inputBlocklen  = inputBlocklen;
  queuedCommand.outputBlocklen = outputBlocklen;
  mCommandQueue.push(queuedCommand);

  return eIasOk;
}


/*****************************************************************************
 * @brief Set the mode of the output buffer.
 *****************************************************************************
 */
void IasSrcFarrow::setBufferMode(IasBufferMode bufferMode)
{
  mBufferMode = bufferMode;
}


/*****************************************************************************
 * @brief Get the output gain.
 *****************************************************************************
 */
float IasSrcFarrow::getOutputGain()
{
  return IASSRCFARROWCONFIG_OUTPUT_GAIN;
}


/*****************************************************************************
 * @brief Reset function.
 *****************************************************************************
 */
IasSrcFarrow::IasResult IasSrcFarrow::reset()
{
  if (!mIsInitialized)
  {
    return eIasNotInitialized;
  }

  IasCommandQueueEntry queuedCommand;
  queuedCommand.commandId = eIasReset;
  mCommandQueue.push(queuedCommand);

  return eIasOk;
}


/*****************************************************************************
 * @brief Private method to pop and execute all commands from the queue.
 *****************************************************************************
 */
IasSrcFarrow::IasResult IasSrcFarrow::executeQueuedCommands()
{
  int                  firStatus;
  IasCommandQueueEntry queuedCommand;

  // Execute all commands from the queue.
  while (mCommandQueue.try_pop(queuedCommand))
  {
    switch (queuedCommand.commandId)
    {
      case eIasSetConversionRatio:
      {
        mDetunedMode  = false;
        mDetuneFactor = 1.0;

        mFilterLength = queuedCommand.filterLength;
        mFirFilterMultiChan->setFilterLength(mFilterLength);
        mNumFilters   = queuedCommand.numFilters;
        mFsRatio = queuedCommand.fsRatio;
        mFsRatioInv = queuedCommand.fsRatioInv;
        mTValue  = 0.0;
        for (uint32_t cnt=0; cnt < mNumFilters; cnt++)
        {
          mImpulseResponses[cnt] = &(queuedCommand.coeff[cnt*mFilterLength]);
        }

#if (IASSRCFARROWCONFIG_USE_SSE)
        // Only for the SSE optimized variant, we need the function pointer for updating the impulse response.
        switch (mNumFilters)
        {
          case 4:
            mUpdateImpulseResponseFunction = &IasSrcFarrow::updateImpulseResponseN4;
            break;
          case 5:
            mUpdateImpulseResponseFunction = &IasSrcFarrow::updateImpulseResponseN5;
            break;
          case 6:
            mUpdateImpulseResponseFunction = &IasSrcFarrow::updateImpulseResponseN6;
            break;
          case 7:
            mUpdateImpulseResponseFunction = &IasSrcFarrow::updateImpulseResponseN7;
            break;
          default:
            mUpdateImpulseResponseFunction = NULL;
            return eIasFailed;
        }
#endif

        // Reset the FIR filters, because the filter length might be different now.
        firStatus = mFirFilterMultiChan->reset();
        if (firStatus != eIasOk)
        {
          return eIasFailed;
        }

        break;
      }

      case eIasDetunePitch:
      {
        // Avoid division by zero. We use IAS_ASSRTs here, because this both
        // parameters have been already checked before.
        IAS_ASSERT(queuedCommand.outputBlocklen > 0);
        IAS_ASSERT(fabs(mFsRatio) >= 1e-10);

        float inputLen          = static_cast<float>(queuedCommand.inputBlocklen);
        float outputLen         = static_cast<float>(queuedCommand.outputBlocklen);

        // In order to avoid round-off issues, which occor if tValue is repeatedly
        // rounded down, we we have to slightly increase the detune factor. We use
        // a correction factor of 1.0+1e-15, which considers that Float64 values
        // have a mantiasa of 52 bit, which results in 2^(-52) = 2.22e-16
        const double cCorrection = 1.0+1e-15;

        mDetuneFactor = cCorrection * inputLen / (outputLen * mFsRatio);
        mTValue       = 0.0;
        mDetunedMode  = true;
        break;
      }

      case eIasReset:
      {

        firStatus = mFirFilterMultiChan->reset();
        if (firStatus)
        {
          return eIasFailed;
        }

        mRingBufferIndex = 0;
        mTValue = 0.0;
        break;
      }
      default:
      {
        IAS_ASSERT(false);
      }
    }
  }

  return eIasOk;
}


#if !(IASSRCFARROWCONFIG_USE_SSE)  // The normal variant (without SSE optimization)

/*****************************************************************************
 * @brief Process the sample rate converter for one frame; the frame consists
 *        of @a numInputSamples samples within @a numChannels channels.
 *****************************************************************************
 */
template <typename T1, typename T2>
IasSrcFarrow::IasResult IasSrcFarrow::processPushMode(T2                 **outputBuffers,
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
                                                      float         ratioAdjustment)
{
  IAS_ASSERT(outputBuffers != nullptr);
  IAS_ASSERT(inputBuffers  != nullptr);
  IAS_ASSERT(numGeneratedSamples != nullptr);
  IAS_ASSERT(numConsumedSamples  != nullptr);
  IAS_ASSERT(writeIndex != nullptr);

  uint32_t  maxOutputSamples;
  IasResult    status;
  int          firStatus;

  // Verify that numChannels is not too high and that ratioAdjustment is not zero.
  if ((numChannels > mMaxNumChannels) || (ratioAdjustment < 0.01))
  {
    return eIasInvalidParam;
  }

  // If the output buffer works as ring buffer, its length must be greater than 1.
  if ((mBufferMode == eIasRingBufferMode) && (lengthOutputBuffers < 2))
  {
    return eIasInvalidParam;
  }

  status = this->executeQueuedCommands();
  if (status != eIasOk)
  {
    return eIasFailed;
  }

  // Verify that filter parameters have been set.
  if ((mNumFilters == 0) || (mFilterLength == 0))
  {
    return eIasInvalidParam;
  }

  if (mBufferMode == eIasLinearBufferMode)
  {
    // In linear buffer mode, start writing new samples at the beginning of the
    // output buffer.
    mRingBufferIndex = 0;
    maxOutputSamples = lengthOutputBuffers;
  }
  else
  {
    maxOutputSamples = numFreeSamplesRingbuffer(readIndex, mRingBufferIndex, lengthOutputBuffers);
  }

  // Consider the adjustment value for the conversion ratio. This is required
  // for asynchronous operation, if the sample rate converter is combined with
  // a closed-loop controller, which updates the conversion ratio according to
  // the clock skew.
  double currentFsRatio = mFsRatio * ratioAdjustment;

  // If the sample rate converter operates with a detuned pitch, adjust the
  // conversion ratio accordingly and reset the mTValue at the beginning
  // of each block (in order to avoid an accumulation of round-off errors).
  if (mDetunedMode)
  {
    currentFsRatio = currentFsRatio * mDetuneFactor;
    mTValue = 0.0;
  }

  uint32_t  cntOutputSamples  = 0;
  uint32_t  cntInputSamples   = 0;
  uint32_t  currentWriteIndex = mRingBufferIndex;
  float yHorner;
  float tValueFloat;

  while (cntInputSamples < numInputSamples)
  {
    if (mTValue < 1.0)
    {
      // If the output ring buffer does not provide space for one more sample, exit the loop.
      if (cntOutputSamples >= maxOutputSamples)
      {
        break;
      }

      // Generate one output sample. To do this, we have to
      // calculate the time-variant impulse response by polynomial
      // interpolation based on the polyphase impulse responses.
      // This is done by means of Horner's method.
      tValueFloat = static_cast<float>(mTValue);
      switch (mNumFilters)
      {
        case 4:
          for (uint32_t cnt=0; cnt < mFilterLength; cnt++)
          {
            yHorner = mImpulseResponses[3][cnt];
            yHorner = mImpulseResponses[2][cnt] + tValueFloat * yHorner;
            yHorner = mImpulseResponses[1][cnt] + tValueFloat * yHorner;
            yHorner = mImpulseResponses[0][cnt] + tValueFloat * yHorner;
            mTimeVarImpResp[cnt] = yHorner;
          }
          break;
        case 5:
          for (uint32_t cnt=0; cnt < mFilterLength; cnt++)
          {
            yHorner = mImpulseResponses[4][cnt];
            yHorner = mImpulseResponses[3][cnt] + tValueFloat * yHorner;
            yHorner = mImpulseResponses[2][cnt] + tValueFloat * yHorner;
            yHorner = mImpulseResponses[1][cnt] + tValueFloat * yHorner;
            yHorner = mImpulseResponses[0][cnt] + tValueFloat * yHorner;
            mTimeVarImpResp[cnt] = yHorner;
          }
          break;
        case 6:
          for (uint32_t cnt=0; cnt < mFilterLength; cnt++)
          {
            yHorner = mImpulseResponses[5][cnt];
            yHorner = mImpulseResponses[4][cnt] + tValueFloat * yHorner;
            yHorner = mImpulseResponses[3][cnt] + tValueFloat * yHorner;
            yHorner = mImpulseResponses[2][cnt] + tValueFloat * yHorner;
            yHorner = mImpulseResponses[1][cnt] + tValueFloat * yHorner;
            yHorner = mImpulseResponses[0][cnt] + tValueFloat * yHorner;
            mTimeVarImpResp[cnt] = yHorner;
          }
          break;
        case 7:
          for (uint32_t cnt=0; cnt < mFilterLength; cnt++)
          {
            yHorner = mImpulseResponses[6][cnt];
            yHorner = mImpulseResponses[5][cnt] + tValueFloat * yHorner;
            yHorner = mImpulseResponses[4][cnt] + tValueFloat * yHorner;
            yHorner = mImpulseResponses[3][cnt] + tValueFloat * yHorner;
            yHorner = mImpulseResponses[2][cnt] + tValueFloat * yHorner;
            yHorner = mImpulseResponses[1][cnt] + tValueFloat * yHorner;
            yHorner = mImpulseResponses[0][cnt] + tValueFloat * yHorner;
            mTimeVarImpResp[cnt] = yHorner;
          }
          break;
        default:
          for (uint32_t cnt=0; cnt < mFilterLength; cnt++)
          {
            mTimeVarImpResp[cnt] = mImpulseResponses[0][cnt];
          }
          break;
      }

      // Load the time-variant impulse response into the multi-channel filter.
      firStatus = mFirFilterMultiChan->setImpulseResponse((const float**)&mTimeVarImpResp, mFilterLength);
      if (firStatus)
      {
        return eIasFailed;
      }
      firStatus = mFirFilterMultiChan->multiInputProcessSample(outputBuffers,
                                                               currentWriteIndex * outputStride,
                                                               numChannels);
      if (firStatus)
      {
        return eIasFailed;
      }
      currentWriteIndex = incrementBufferIndex(currentWriteIndex, lengthOutputBuffers);
      cntOutputSamples++;
      mTValue = mTValue + currentFsRatio;
    }
    else
    {
      // Consume one input sample
      mTValue = mTValue - 1.0;
      firStatus = mFirFilterMultiChan->multiInputInsertSample(inputBuffers,
                                                              cntInputSamples * inputStride,
                                                              numChannels);
      if (firStatus)
      {
        return eIasFailed;
      }
      cntInputSamples++;
    }
  }

  *numGeneratedSamples = cntOutputSamples;
  *numConsumedSamples  = cntInputSamples;
  *writeIndex          = currentWriteIndex;
  mRingBufferIndex     = currentWriteIndex;
  return eIasOk;
}


#else // !(IASSRCFARROWCONFIG_USE_SSE) // Now following... the SSE-optimized variant

/*****************************************************************************
 * @brief Process the sample rate converter for one frame; the frame consists
 *        of @a numInputSamples samples within @a numChannels channels.
 *        This is the SSE-optimized variant.
 *****************************************************************************
 */
template <typename T1, typename T2>
IasSrcFarrow::IasResult IasSrcFarrow::processPushMode(T2                 **outputBuffers,
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
                                                      float         ratioAdjustment)
{
  IAS_ASSERT(outputBuffers != nullptr);
  IAS_ASSERT(inputBuffers  != nullptr);
  IAS_ASSERT(numGeneratedSamples != nullptr);
  IAS_ASSERT(numConsumedSamples  != nullptr);
  IAS_ASSERT(writeIndex != nullptr);

  uint32_t  maxOutputSamples;
  IasResult    status;
  int          firStatus;

  // Verify that numChannels is not too high and that ratioAdjustment is not zero.
  if ((numChannels > mMaxNumChannels) || (ratioAdjustment < 0.01))
  {
    return eIasInvalidParam;
  }

  // If the output buffer works as ring buffer, its length must be greater than 1.
  if ((mBufferMode == eIasRingBufferMode) && (lengthOutputBuffers < 2))
  {
    return eIasInvalidParam;
  }

  status = this->executeQueuedCommands();
  if (status != eIasOk)
  {
    return eIasFailed;
  }

  // Verify that filter parameters have been set.
  if ((mNumFilters == 0) || (mFilterLength == 0) || (mUpdateImpulseResponseFunction == NULL))
  {
    return eIasInvalidParam;
  }

  if (mBufferMode == eIasLinearBufferMode)
  {
    // In linear buffer mode, start writing new samples at the beginning of the
    // output buffer.
    mRingBufferIndex = 0;
    maxOutputSamples = lengthOutputBuffers;
  }
  else
  {
    maxOutputSamples = numFreeSamplesRingbuffer(readIndex, mRingBufferIndex, lengthOutputBuffers);
  }

  float * __restrict       timeVarImpResp = mTimeVarImpResp;

  // Consider the adjustment value for the conversion ratio. This is required
  // for asynchronous operation, if the sample rate converter is combined with
  // a closed-loop controller, which updates the conversion ratio according to
  // the clock skew.
  double currentFsRatio = mFsRatio * ratioAdjustment;

  // If the sample rate converter operates with a detuned pitch, adjust the
  // conversion ratio accordingly and reset the mTValue at the beginning
  // of each block (in order to avoid an accumulation of round-off errors).
  if (mDetunedMode)
  {
    currentFsRatio = currentFsRatio * mDetuneFactor;
    mTValue = 0.0;
  }

  uint32_t  cntOutputSamples  = 0;
  uint32_t  cntInputSamples   = 0;
  uint32_t  currentWriteIndex = mRingBufferIndex;
  __m128 const cZero             = _mm_setzero_ps();

  while (cntInputSamples < numInputSamples)
  {
    if (mTValue < 1.0)
    {
      // If the output ring buffer does not provide space for one more sample, exit the loop.
      if (cntOutputSamples >= maxOutputSamples)
      {
        break;
      }

      // Generate one output sample. To do this, we have to
      // calculate the time-variant impulse response by polynomial
      // interpolation based on the polyphase impulse responses.
      // This is done by means of Horner's method.

      // Fill the head and the tail of the time variant impulse response with zeros.
      // This is required, since the time variant impulse response might be
      // shifted by 0, 1, 2, or 3 samples, depending on getPaddingForSSE().
      _mm_store_ps(&timeVarImpResp[0], cZero);
      _mm_store_ps(&timeVarImpResp[mFilterLength], cZero);

      // Calculate the time-variant impulse response. The <mFilterLength>
      // coefficients are stored in the destination buffer using a shift
      // 0, 1, 2, or 3 samples, depending on getPaddingForSSE()
      (this->*mUpdateImpulseResponseFunction)(&mTimeVarImpResp[mFirFilterMultiChan->getPaddingForSSE()],
                                              static_cast<float>(mTValue));

      // Load the time-variant impulse response into the multi-channel filter.
      firStatus = mFirFilterMultiChan->setImpulseResponse((const float**)&timeVarImpResp, mFilterLength);
      if (firStatus)
      {
        return eIasFailed;
      }

      // Execute the FIR filters, generate one output sample for each channel.
      firStatus = mFirFilterMultiChan->multiInputProcessSample(outputBuffers,
                                                               currentWriteIndex * outputStride,
                                                               numChannels);
      if (firStatus)
      {
        return eIasFailed;
      }
      currentWriteIndex = incrementBufferIndex(currentWriteIndex, lengthOutputBuffers);
      cntOutputSamples++;
      mTValue = mTValue + currentFsRatio;
    }
    else
    {
      // Consume one input sample
      mTValue = mTValue - 1.0;
      firStatus = mFirFilterMultiChan->multiInputInsertSample(inputBuffers,
                                                              cntInputSamples * inputStride,
                                                              numChannels);
      if (firStatus)
      {
        return eIasFailed;
      }
      cntInputSamples++;
    }
  }

  *numGeneratedSamples = cntOutputSamples;
  *numConsumedSamples  = cntInputSamples;
  *writeIndex          = currentWriteIndex;
  mRingBufferIndex     = currentWriteIndex;
  return eIasOk;
}

#endif // !(IASSRCFARROWCONFIG_USE_SSE)


/*
 * Tell the compiler that we need this template/function for float, int32_t, and int16_t
 */
template IasSrcFarrow::IasResult IasSrcFarrow::processPushMode<float,float>(float       **outputBuffers,
                                                                                          float const **inputBuffers,
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

template IasSrcFarrow::IasResult IasSrcFarrow::processPushMode<float,int32_t>(int32_t         **outputBuffers,
                                                                                        float const **inputBuffers,
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

template IasSrcFarrow::IasResult IasSrcFarrow::processPushMode<float,int16_t>(int16_t         **outputBuffers,
                                                                                        float const **inputBuffers,
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

template IasSrcFarrow::IasResult IasSrcFarrow::processPushMode<int32_t, float>(float     **outputBuffers,
                                                                                         int32_t const **inputBuffers,
                                                                                         uint32_t        outputStride,
                                                                                         uint32_t        inputStride,
                                                                                         uint32_t       *numGeneratedSamples,
                                                                                         uint32_t       *numConsumedSamples,
                                                                                         uint32_t       *writeIndex,
                                                                                         uint32_t        readIndex,
                                                                                         uint32_t        lengthOutputBuffers,
                                                                                         uint32_t        numInputSamples,
                                                                                         uint32_t        numChannels,
                                                                                         float       ratioAdjustment);

template IasSrcFarrow::IasResult IasSrcFarrow::processPushMode<int32_t, int32_t>(int32_t       **outputBuffers,
                                                                                       int32_t const **inputBuffers,
                                                                                       uint32_t        outputStride,
                                                                                       uint32_t        inputStride,
                                                                                       uint32_t       *numGeneratedSamples,
                                                                                       uint32_t       *numConsumedSamples,
                                                                                       uint32_t       *writeIndex,
                                                                                       uint32_t        readIndex,
                                                                                       uint32_t        lengthOutputBuffers,
                                                                                       uint32_t        numInputSamples,
                                                                                       uint32_t        numChannels,
                                                                                       float       ratioAdjustment);

template IasSrcFarrow::IasResult IasSrcFarrow::processPushMode<int32_t, int16_t>(int16_t       **outputBuffers,
                                                                                       int32_t const **inputBuffers,
                                                                                       uint32_t        outputStride,
                                                                                       uint32_t        inputStride,
                                                                                       uint32_t       *numGeneratedSamples,
                                                                                       uint32_t       *numConsumedSamples,
                                                                                       uint32_t       *writeIndex,
                                                                                       uint32_t        readIndex,
                                                                                       uint32_t        lengthOutputBuffers,
                                                                                       uint32_t        numInputSamples,
                                                                                       uint32_t        numChannels,
                                                                                       float       ratioAdjustment);

template IasSrcFarrow::IasResult IasSrcFarrow::processPushMode<int16_t, int16_t>(int16_t       **outputBuffers,
                                                                                       int16_t const **inputBuffers,
                                                                                       uint32_t        outputStride,
                                                                                       uint32_t        inputStride,
                                                                                       uint32_t       *numGeneratedSamples,
                                                                                       uint32_t       *numConsumedSamples,
                                                                                       uint32_t       *writeIndex,
                                                                                       uint32_t        readIndex,
                                                                                       uint32_t        lengthOutputBuffers,
                                                                                       uint32_t        numInputSamples,
                                                                                       uint32_t        numChannels,
                                                                                       float       ratioAdjustment);

template IasSrcFarrow::IasResult IasSrcFarrow::processPushMode<int16_t, int32_t>(int32_t       **outputBuffers,
                                                                                       int16_t const **inputBuffers,
                                                                                       uint32_t        outputStride,
                                                                                       uint32_t        inputStride,
                                                                                       uint32_t       *numGeneratedSamples,
                                                                                       uint32_t       *numConsumedSamples,
                                                                                       uint32_t       *writeIndex,
                                                                                       uint32_t        readIndex,
                                                                                       uint32_t        lengthOutputBuffers,
                                                                                       uint32_t        numInputSamples,
                                                                                       uint32_t        numChannels,
                                                                                       float       ratioAdjustment);

template IasSrcFarrow::IasResult IasSrcFarrow::processPushMode<int16_t, float>(float     **outputBuffers,
                                                                                         int16_t const **inputBuffers,
                                                                                         uint32_t        outputStride,
                                                                                         uint32_t        inputStride,
                                                                                         uint32_t       *numGeneratedSamples,
                                                                                         uint32_t       *numConsumedSamples,
                                                                                         uint32_t       *writeIndex,
                                                                                         uint32_t        readIndex,
                                                                                         uint32_t        lengthOutputBuffers,
                                                                                         uint32_t        numInputSamples,
                                                                                         uint32_t        numChannels,
                                                                                         float       ratioAdjustment);


#if !(IASSRCFARROWCONFIG_USE_SSE)  // The normal variant (without SSE optimization)

/*****************************************************************************
 * @brief Process the sample rate converter and generate one frame of
 *        @a numOutputSamples samples within @a numChannels channels.
 *****************************************************************************
 */
template <typename T1, typename T2>
IasSrcFarrow::IasResult IasSrcFarrow::processPullMode(T2                 **outputBuffers,
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
                                                      float         ratioAdjustment)
{
  IAS_ASSERT(outputBuffers != nullptr);
  IAS_ASSERT(inputBuffers  != nullptr);
  IAS_ASSERT(numGeneratedSamples != nullptr);
  IAS_ASSERT(numConsumedSamples  != nullptr);
  IAS_ASSERT(readIndex != nullptr);

  uint32_t  maxInputSamples;
  IasResult    status;
  int          firStatus;

  // Verify that numChannels is not too high and that ratioAdjustment is not zero.
  if ((numChannels > mMaxNumChannels) || (ratioAdjustment < 0.01))
  {
    return eIasInvalidParam;
  }

  // If the input buffer works as ring buffer, its length must be greater than 1.
  if ((mBufferMode == eIasRingBufferMode) && (lengthInputBuffers < 2))
  {
    return eIasInvalidParam;
  }

  status = this->executeQueuedCommands();
  if (status)
  {
    return eIasFailed;
  }

  // Verify that filter parameters have been set.
  if ((mNumFilters == 0) || (mFilterLength == 0))
  {
    return eIasInvalidParam;
  }

  if (mBufferMode == eIasLinearBufferMode)
  {
    // In linear buffer mode, start reading input samples from the beginning of the input
    // buffer.
    mRingBufferIndex = 0;
    maxInputSamples = lengthInputBuffers;
  }
  else
  {
    maxInputSamples = numFilledSamplesRingbuffer(writeIndex, mRingBufferIndex, lengthInputBuffers);
  }

  // Consider the adjustment value for the conversion ratio. This is required
  // for asynchronous operation, if the sample rate converter is combined with
  // a closed-loop controller, which updates the conversion ratio according to
  // the clock skew.
  // In pull-mode operation, we still have fsRatio = inputRate/outputRate
  // while ratioAdjustment = fs1 / fs2 = outputRate/inputRate. Therefore, we
  // have to multiply ratioAdjustment with FsRatioInv.
  double currentFsRatioInv = mFsRatioInv * ratioAdjustment;

  // If the sample rate converter operates with a detuned pitch, adjust the
  // conversion ratio accordingly and reset the mTValue at the beginning
  // of each block (in order to avoid an accumulation of round-off errors).
  if (mDetunedMode)
  {
    currentFsRatioInv = currentFsRatioInv * mDetuneFactor;
    mTValue = 0.0;
  }

  float currentFsRatio   = 1.0f / static_cast<float>(currentFsRatioInv);
  uint32_t  cntOutputSamples = 0;
  uint32_t  cntInputSamples  = 0;
  uint32_t  currentReadIndex = mRingBufferIndex;
  float yHorner;
  float tValueFloat;

  while (cntOutputSamples < numOutputSamples)
  {
    if (mTValue < 1.0)
    {
      // Consume one input sample.
      // If the input ring buffer does not provide any more samples, exit the loop.
      if (cntInputSamples >= maxInputSamples)
      {
        break;
      }

      // Resync mTValueHat to mTValue and update mTValue to match the push-mode update.
      mTValueHat = (1.0f - static_cast<float>(mTValue)) * currentFsRatio;
      mTValue = mTValue + currentFsRatioInv;

      firStatus = mFirFilterMultiChan->multiInputInsertSample(inputBuffers,
                                                           currentReadIndex * inputStride,
                                                           numChannels);
      currentReadIndex = incrementBufferIndex(currentReadIndex, lengthInputBuffers);
      cntInputSamples++;

      if (firStatus)
      {
        return eIasFailed;
      }
    }
    else
    {
      // Generate one output sample. To do this, we have to
      // calculate the time-variant impulse response by polynomial
      // interpolation based on the polyphase impulse responses.
      // This is done by means of Horner's method.
      tValueFloat = mTValueHat;

      switch (mNumFilters)
      {
        case 4:
          for (uint32_t cnt=0; cnt < mFilterLength; cnt++)
          {
            yHorner = mImpulseResponses[3][cnt];
            yHorner = mImpulseResponses[2][cnt] + tValueFloat * yHorner;
            yHorner = mImpulseResponses[1][cnt] + tValueFloat * yHorner;
            yHorner = mImpulseResponses[0][cnt] + tValueFloat * yHorner;
            mTimeVarImpResp[cnt] = yHorner;
          }
          break;
        case 5:
          for (uint32_t cnt=0; cnt < mFilterLength; cnt++)
          {
            yHorner = mImpulseResponses[4][cnt];
            yHorner = mImpulseResponses[3][cnt] + tValueFloat * yHorner;
            yHorner = mImpulseResponses[2][cnt] + tValueFloat * yHorner;
            yHorner = mImpulseResponses[1][cnt] + tValueFloat * yHorner;
            yHorner = mImpulseResponses[0][cnt] + tValueFloat * yHorner;
            mTimeVarImpResp[cnt] = yHorner;
          }
          break;
        case 6:
          for (uint32_t cnt=0; cnt < mFilterLength; cnt++)
          {
            yHorner = mImpulseResponses[5][cnt];
            yHorner = mImpulseResponses[4][cnt] + tValueFloat * yHorner;
            yHorner = mImpulseResponses[3][cnt] + tValueFloat * yHorner;
            yHorner = mImpulseResponses[2][cnt] + tValueFloat * yHorner;
            yHorner = mImpulseResponses[1][cnt] + tValueFloat * yHorner;
            yHorner = mImpulseResponses[0][cnt] + tValueFloat * yHorner;
            mTimeVarImpResp[cnt] = yHorner;
          }
          break;
        case 7:
          for (uint32_t cnt=0; cnt < mFilterLength; cnt++)
          {
            yHorner = mImpulseResponses[6][cnt];
            yHorner = mImpulseResponses[5][cnt] + tValueFloat * yHorner;
            yHorner = mImpulseResponses[4][cnt] + tValueFloat * yHorner;
            yHorner = mImpulseResponses[3][cnt] + tValueFloat * yHorner;
            yHorner = mImpulseResponses[2][cnt] + tValueFloat * yHorner;
            yHorner = mImpulseResponses[1][cnt] + tValueFloat * yHorner;
            yHorner = mImpulseResponses[0][cnt] + tValueFloat * yHorner;
            mTimeVarImpResp[cnt] = yHorner;
          }
          break;
        default:
          for (uint32_t cnt=0; cnt < mFilterLength; cnt++)
          {
            mTimeVarImpResp[cnt] = mImpulseResponses[0][cnt];
          }
          break;
      }

      // Load the time-variant impulse response into the multi-channel filter.
      firStatus = mFirFilterMultiChan->setImpulseResponse((const float**)&mTimeVarImpResp, mFilterLength);
      if (firStatus)
      {
        return eIasFailed;
      }

      // Execute the FIR filters, generate one output sample for each channel.
      firStatus = mFirFilterMultiChan->multiInputProcessSample(outputBuffers,
                                                            cntOutputSamples * outputStride,
                                                            numChannels);
      if (firStatus)
      {
        return eIasFailed;
      }
      cntOutputSamples++;
      mTValueHat = mTValueHat + currentFsRatio;
      mTValue    = mTValue - 1.0;
    }
  }

  *numGeneratedSamples = cntOutputSamples;
  *numConsumedSamples  = cntInputSamples;
  *readIndex           = currentReadIndex;
  mRingBufferIndex     = currentReadIndex;
  return eIasOk;
}


#else // !(IASSRCFARROWCONFIG_USE_SSE) // Now following... the SSE-optimized variant

/*****************************************************************************
 * @brief Process the sample rate converter and generate one frame of
 *        @a numOutputSamples samples within @a numChannels channels.
 *        This is the SSE-optimized variant.
 *****************************************************************************
 */
template <typename T1, typename T2>
IasSrcFarrow::IasResult IasSrcFarrow::processPullMode(T2                **outputBuffers,
                                                      T1          const **inputBuffers,
                                                      uint32_t         outputStride,
                                                      uint32_t         inputStride,
                                                      uint32_t        *numGeneratedSamples,
                                                      uint32_t        *numConsumedSamples,
                                                      uint32_t        *readIndex,
                                                      uint32_t         writeIndex,
                                                      uint32_t         lengthInputBuffers,
                                                      uint32_t         numOutputSamples,
                                                      uint32_t         numChannels,
                                                      float        ratioAdjustment)
{
  IAS_ASSERT(outputBuffers != nullptr);
  IAS_ASSERT(inputBuffers != nullptr);
  IAS_ASSERT(numGeneratedSamples != nullptr);
  IAS_ASSERT(numConsumedSamples  != nullptr);
  IAS_ASSERT(readIndex != nullptr);

  uint32_t  maxInputSamples;
  IasResult    status;
  int          firStatus;

  // Verify that numChannels is not too high and that ratioAdjustment is not zero.
  if ((numChannels > mMaxNumChannels) || (ratioAdjustment < 0.01))
  {
    return eIasInvalidParam;
  }

  // If the input buffer works as ring buffer, its length must be greater than 1.
  if ((mBufferMode == eIasRingBufferMode) && (lengthInputBuffers < 2))
  {
    return eIasInvalidParam;
  }

  status = executeQueuedCommands();
  if (status != eIasOk)
  {
    return eIasFailed;
  }

  // Verify that filter parameters have been set.
  if ((mNumFilters == 0) || (mFilterLength == 0) || (mUpdateImpulseResponseFunction == NULL))
  {
    return eIasInvalidParam;
  }

  float * __restrict timeVarImpResp = mTimeVarImpResp;

  if (mBufferMode == eIasLinearBufferMode)
  {
    // In linear buffer mode, start reading input samples from the beginning of the input
    // buffer.
    mRingBufferIndex = 0;
    maxInputSamples = lengthInputBuffers;
  }
  else
  {
    maxInputSamples = numFilledSamplesRingbuffer(writeIndex, mRingBufferIndex, lengthInputBuffers);
  }

  // Consider the adjustment value for the conversion ratio. This is required
  // for asynchronous operation, if the sample rate converter is combined with
  // a closed-loop controller, which updates the conversion ratio according to
  // the clock skew.
  // In pull-mode operation, we still have fsRatio = inputRate/outputRate
  // while ratioAdjustment = fs1 / fs2 = outputRate/inputRate. Therefore, we
  // have to multiply ratioAdjustment with FsRatioInv.
  double currentFsRatioInv = mFsRatioInv * ratioAdjustment;

  // If the sample rate converter operates with a detuned pitch, adjust the
  // conversion ratio accordingly and reset the mTValue at the beginning
  // of each block (in order to avoid an accumulation of round-off errors).
  if (mDetunedMode)
  {
    currentFsRatioInv = currentFsRatioInv * mDetuneFactor;
    mTValue = 0.0;
  }

  float currentFsRatio   = 1.0f / static_cast<float>(currentFsRatioInv);
  uint32_t  cntOutputSamples = 0;
  uint32_t  cntInputSamples  = 0;
  uint32_t  currentReadIndex = mRingBufferIndex;
  __m128 const cZero            = _mm_setzero_ps();

  while (cntOutputSamples < numOutputSamples)
  {
    if (mTValue < 1.0)
    {
      // Consume one input sample.
      // If the input ring buffer does not provide any more samples, exit the loop.
      if (cntInputSamples >= maxInputSamples)
      {
        break;
      }

      // Resync mTValueHat to mTValue and update mTValue to match the push-mode update.
      mTValueHat = (1.0f - static_cast<float>(mTValue)) * currentFsRatio;
      mTValue = mTValue + currentFsRatioInv;

      firStatus = mFirFilterMultiChan->multiInputInsertSample(inputBuffers,
                                                              currentReadIndex * inputStride,
                                                              numChannels);
      currentReadIndex = incrementBufferIndex(currentReadIndex, lengthInputBuffers);
      cntInputSamples++;

      if (firStatus)
      {
        return eIasFailed;
      }
    }
    else
    {
      // Generate one output sample. To do this, we have to
      // calculate the time-variant impulse response by polynomial
      // interpolation based on the polyphase impulse responses.
      // This is done by means of Horner's method.

      // Fill the head and the tail of the time variant impulse response with zeros.
      // This is required, since the time variant impulse response might be
      // shifted by 0, 1, 2, or 3 samples, depending on getPaddingForSSE().
      _mm_store_ps(&timeVarImpResp[0], cZero);
      _mm_store_ps(&timeVarImpResp[mFilterLength], cZero);

      // Calculate the time-variant impulse response. The <mFilterLength>
      // coefficients are stored in the destination buffer using a shift
      // 0, 1, 2, or 3 samples, depending on getPaddingForSSE()
      (this->*mUpdateImpulseResponseFunction)(&mTimeVarImpResp[mFirFilterMultiChan->getPaddingForSSE()], mTValueHat);

      // Load the time-variant impulse response into the multi-channel filter.
      firStatus = mFirFilterMultiChan->setImpulseResponse((const float**)&timeVarImpResp, mFilterLength);
      if (firStatus)
      {
        return eIasFailed;
      }

      // Execute the FIR filters, generate one output sample for each channel.
      firStatus = mFirFilterMultiChan->multiInputProcessSample(outputBuffers,
                                                            cntOutputSamples * outputStride,
                                                            numChannels);
      if (firStatus)
      {
        return eIasFailed;
      }
      cntOutputSamples++;
      mTValueHat = mTValueHat + currentFsRatio;
      mTValue    = mTValue - 1.0;
    }
  }

  *numGeneratedSamples = cntOutputSamples;
  *numConsumedSamples  = cntInputSamples;
  *readIndex           = currentReadIndex;
  mRingBufferIndex     = currentReadIndex;
  return eIasOk;
}

#endif // !(IASSRCFARROWCONFIG_USE_SSE)


/*
 * Tell the compiler that we need this template/function for float, int32_t, and int16_t
 */
template IasSrcFarrow::IasResult IasSrcFarrow::processPullMode<float,float>(float       **outputBuffers,
                                                                                          float const **inputBuffers,
                                                                                          uint32_t          outputStride,
                                                                                          uint32_t          inputStride,
                                                                                          uint32_t         *numGeneratedSamples,
                                                                                          uint32_t         *numConsumedSamples,
                                                                                          uint32_t         *readIndex,
                                                                                          uint32_t          writeIndex,
                                                                                          uint32_t          lengthOutputBuffers,
                                                                                          uint32_t          numInputSamples,
                                                                                          uint32_t          numChannels,
                                                                                          float         ratioAdjustment);

template IasSrcFarrow::IasResult IasSrcFarrow::processPullMode<float,int32_t>(int32_t         **outputBuffers,
                                                                                        float const **inputBuffers,
                                                                                        uint32_t          outputStride,
                                                                                        uint32_t          inputStride,
                                                                                        uint32_t         *numGeneratedSamples,
                                                                                        uint32_t         *numConsumedSamples,
                                                                                        uint32_t         *readIndex,
                                                                                        uint32_t          writeIndex,
                                                                                        uint32_t          lengthOutputBuffers,
                                                                                        uint32_t          numInputSamples,
                                                                                        uint32_t          numChannels,
                                                                                        float         ratioAdjustment);

template IasSrcFarrow::IasResult IasSrcFarrow::processPullMode<float,int16_t>(int16_t         **outputBuffers,
                                                                                        float const **inputBuffers,
                                                                                        uint32_t          outputStride,
                                                                                        uint32_t          inputStride,
                                                                                        uint32_t         *numGeneratedSamples,
                                                                                        uint32_t         *numConsumedSamples,
                                                                                        uint32_t         *readIndex,
                                                                                        uint32_t          writeIndex,
                                                                                        uint32_t          lengthOutputBuffers,
                                                                                        uint32_t          numInputSamples,
                                                                                        uint32_t          numChannels,
                                                                                        float         ratioAdjustment);

template IasSrcFarrow::IasResult IasSrcFarrow::processPullMode<int32_t, float>(float     **outputBuffers,
                                                                                         int32_t const **inputBuffers,
                                                                                         uint32_t        outputStride,
                                                                                         uint32_t        inputStride,
                                                                                         uint32_t       *numGeneratedSamples,
                                                                                         uint32_t       *numConsumedSamples,
                                                                                         uint32_t       *readIndex,
                                                                                         uint32_t        writeIndex,
                                                                                         uint32_t        lengthOutputBuffers,
                                                                                         uint32_t        numInputSamples,
                                                                                         uint32_t        numChannels,
                                                                                         float       ratioAdjustment);

template IasSrcFarrow::IasResult IasSrcFarrow::processPullMode<int32_t, int32_t>(int32_t       **outputBuffers,
                                                                                       int32_t const **inputBuffers,
                                                                                       uint32_t        outputStride,
                                                                                       uint32_t        inputStride,
                                                                                       uint32_t       *numGeneratedSamples,
                                                                                       uint32_t       *numConsumedSamples,
                                                                                       uint32_t       *readIndex,
                                                                                       uint32_t        writeIndex,
                                                                                       uint32_t        lengthOutputBuffers,
                                                                                       uint32_t        numInputSamples,
                                                                                       uint32_t        numChannels,
                                                                                       float       ratioAdjustment);

template IasSrcFarrow::IasResult IasSrcFarrow::processPullMode<int32_t, int16_t>(int16_t       **outputBuffers,
                                                                                       int32_t const **inputBuffers,
                                                                                       uint32_t        outputStride,
                                                                                       uint32_t        inputStride,
                                                                                       uint32_t       *numGeneratedSamples,
                                                                                       uint32_t       *numConsumedSamples,
                                                                                       uint32_t       *readIndex,
                                                                                       uint32_t        writeIndex,
                                                                                       uint32_t        lengthOutputBuffers,
                                                                                       uint32_t        numInputSamples,
                                                                                       uint32_t        numChannels,
                                                                                       float       ratioAdjustment);

template IasSrcFarrow::IasResult IasSrcFarrow::processPullMode<int16_t, int16_t>(int16_t       **outputBuffers,
                                                                                       int16_t const **inputBuffers,
                                                                                       uint32_t        outputStride,
                                                                                       uint32_t        inputStride,
                                                                                       uint32_t       *numGeneratedSamples,
                                                                                       uint32_t       *numConsumedSamples,
                                                                                       uint32_t       *readIndex,
                                                                                       uint32_t        writeIndex,
                                                                                       uint32_t        lengthOutputBuffers,
                                                                                       uint32_t        numInputSamples,
                                                                                       uint32_t        numChannels,
                                                                                       float       ratioAdjustment);

template IasSrcFarrow::IasResult IasSrcFarrow::processPullMode<int16_t, int32_t>(int32_t       **outputBuffers,
                                                                                       int16_t const **inputBuffers,
                                                                                       uint32_t        outputStride,
                                                                                       uint32_t        inputStride,
                                                                                       uint32_t       *numGeneratedSamples,
                                                                                       uint32_t       *numConsumedSamples,
                                                                                       uint32_t       *readIndex,
                                                                                       uint32_t        writeIndex,
                                                                                       uint32_t        lengthOutputBuffers,
                                                                                       uint32_t        numInputSamples,
                                                                                       uint32_t        numChannels,
                                                                                       float       ratioAdjustment);

template IasSrcFarrow::IasResult IasSrcFarrow::processPullMode<int16_t, float>(float     **outputBuffers,
                                                                                         int16_t const **inputBuffers,
                                                                                         uint32_t        outputStride,
                                                                                         uint32_t        inputStride,
                                                                                         uint32_t       *numGeneratedSamples,
                                                                                         uint32_t       *numConsumedSamples,
                                                                                         uint32_t       *readIndex,
                                                                                         uint32_t        writeIndex,
                                                                                         uint32_t        lengthOutputBuffers,
                                                                                         uint32_t        numInputSamples,
                                                                                         uint32_t        numChannels,
                                                                                         float       ratioAdjustment);

#if (IASSRCFARROWCONFIG_USE_SSE)  // The following functions are required only for the SSE optimized variant.

/**
 *  Update the time variant impulse response, using N=4 prototype impulse responses.
 */
void IasSrcFarrow::updateImpulseResponseN4(float *destination, float tValue) const
{
  float       * __restrict dst = destination;

  float const * __restrict impulseResponse0 = mImpulseResponses[0];
  float const * __restrict impulseResponse1 = mImpulseResponses[1];
  float const * __restrict impulseResponse2 = mImpulseResponses[2];
  float const * __restrict impulseResponse3 = mImpulseResponses[3];

  __m128 const       tValue_pack = _mm_load1_ps(&tValue);
  __m128 yHorner_pack1;
  __m128 yHorner_pack2;
  __m128 yHorner_pack3;
  __m128 yHorner_pack4;

  for (uint32_t cnt=0; cnt < mFilterLength; cnt+=16)
  {
    yHorner_pack1 = _mm_load_ps(impulseResponse3);
    yHorner_pack2 = _mm_load_ps(impulseResponse3+4);
    yHorner_pack3 = _mm_load_ps(impulseResponse3+8);
    yHorner_pack4 = _mm_load_ps(impulseResponse3+12);

    yHorner_pack1 = _mm_add_ps(_mm_load_ps(impulseResponse2),    _mm_mul_ps(tValue_pack, yHorner_pack1));
    yHorner_pack2 = _mm_add_ps(_mm_load_ps(impulseResponse2+4),  _mm_mul_ps(tValue_pack, yHorner_pack2));
    yHorner_pack3 = _mm_add_ps(_mm_load_ps(impulseResponse2+8),  _mm_mul_ps(tValue_pack, yHorner_pack3));
    yHorner_pack4 = _mm_add_ps(_mm_load_ps(impulseResponse2+12), _mm_mul_ps(tValue_pack, yHorner_pack4));

    yHorner_pack1 = _mm_add_ps(_mm_load_ps(impulseResponse1),    _mm_mul_ps(tValue_pack, yHorner_pack1));
    yHorner_pack2 = _mm_add_ps(_mm_load_ps(impulseResponse1+4),  _mm_mul_ps(tValue_pack, yHorner_pack2));
    yHorner_pack3 = _mm_add_ps(_mm_load_ps(impulseResponse1+8),  _mm_mul_ps(tValue_pack, yHorner_pack3));
    yHorner_pack4 = _mm_add_ps(_mm_load_ps(impulseResponse1+12), _mm_mul_ps(tValue_pack, yHorner_pack4));

    yHorner_pack1 = _mm_add_ps(_mm_load_ps(impulseResponse0),    _mm_mul_ps(tValue_pack, yHorner_pack1));
    yHorner_pack2 = _mm_add_ps(_mm_load_ps(impulseResponse0+4),  _mm_mul_ps(tValue_pack, yHorner_pack2));
    yHorner_pack3 = _mm_add_ps(_mm_load_ps(impulseResponse0+8),  _mm_mul_ps(tValue_pack, yHorner_pack3));
    yHorner_pack4 = _mm_add_ps(_mm_load_ps(impulseResponse0+12), _mm_mul_ps(tValue_pack, yHorner_pack4));

    // unaligned store
    _mm_storeu_ps(dst,    yHorner_pack1);
    _mm_storeu_ps(dst+4,  yHorner_pack2);
    _mm_storeu_ps(dst+8,  yHorner_pack3);
    _mm_storeu_ps(dst+12, yHorner_pack4);

    // Update pointers
    impulseResponse0 += 16;
    impulseResponse1 += 16;
    impulseResponse2 += 16;
    impulseResponse3 += 16;
    dst              += 16;
  }
}

/**
 *  Update the time variant impulse response, using N=5 prototype impulse responses.
 */
void IasSrcFarrow::updateImpulseResponseN5(float *destination, float tValue) const
{
  float       * __restrict dst = destination;

  float const * __restrict impulseResponse0 = mImpulseResponses[0];
  float const * __restrict impulseResponse1 = mImpulseResponses[1];
  float const * __restrict impulseResponse2 = mImpulseResponses[2];
  float const * __restrict impulseResponse3 = mImpulseResponses[3];
  float const * __restrict impulseResponse4 = mImpulseResponses[4];

  __m128 const       tValue_pack = _mm_load1_ps(&tValue);
  __m128 yHorner_pack1;
  __m128 yHorner_pack2;
  __m128 yHorner_pack3;
  __m128 yHorner_pack4;

  for (uint32_t cnt=0; cnt < mFilterLength; cnt+=16)
  {
    yHorner_pack1 = _mm_load_ps(impulseResponse4);
    yHorner_pack2 = _mm_load_ps(impulseResponse4+4);
    yHorner_pack3 = _mm_load_ps(impulseResponse4+8);
    yHorner_pack4 = _mm_load_ps(impulseResponse4+12);

    yHorner_pack1 = _mm_add_ps(_mm_load_ps(impulseResponse3),    _mm_mul_ps(tValue_pack, yHorner_pack1));
    yHorner_pack2 = _mm_add_ps(_mm_load_ps(impulseResponse3+4),  _mm_mul_ps(tValue_pack, yHorner_pack2));
    yHorner_pack3 = _mm_add_ps(_mm_load_ps(impulseResponse3+8),  _mm_mul_ps(tValue_pack, yHorner_pack3));
    yHorner_pack4 = _mm_add_ps(_mm_load_ps(impulseResponse3+12), _mm_mul_ps(tValue_pack, yHorner_pack4));

    yHorner_pack1 = _mm_add_ps(_mm_load_ps(impulseResponse2),    _mm_mul_ps(tValue_pack, yHorner_pack1));
    yHorner_pack2 = _mm_add_ps(_mm_load_ps(impulseResponse2+4),  _mm_mul_ps(tValue_pack, yHorner_pack2));
    yHorner_pack3 = _mm_add_ps(_mm_load_ps(impulseResponse2+8),  _mm_mul_ps(tValue_pack, yHorner_pack3));
    yHorner_pack4 = _mm_add_ps(_mm_load_ps(impulseResponse2+12), _mm_mul_ps(tValue_pack, yHorner_pack4));

    yHorner_pack1 = _mm_add_ps(_mm_load_ps(impulseResponse1),    _mm_mul_ps(tValue_pack, yHorner_pack1));
    yHorner_pack2 = _mm_add_ps(_mm_load_ps(impulseResponse1+4),  _mm_mul_ps(tValue_pack, yHorner_pack2));
    yHorner_pack3 = _mm_add_ps(_mm_load_ps(impulseResponse1+8),  _mm_mul_ps(tValue_pack, yHorner_pack3));
    yHorner_pack4 = _mm_add_ps(_mm_load_ps(impulseResponse1+12), _mm_mul_ps(tValue_pack, yHorner_pack4));

    yHorner_pack1 = _mm_add_ps(_mm_load_ps(impulseResponse0),    _mm_mul_ps(tValue_pack, yHorner_pack1));
    yHorner_pack2 = _mm_add_ps(_mm_load_ps(impulseResponse0+4),  _mm_mul_ps(tValue_pack, yHorner_pack2));
    yHorner_pack3 = _mm_add_ps(_mm_load_ps(impulseResponse0+8),  _mm_mul_ps(tValue_pack, yHorner_pack3));
    yHorner_pack4 = _mm_add_ps(_mm_load_ps(impulseResponse0+12), _mm_mul_ps(tValue_pack, yHorner_pack4));

    // unaligned store
    _mm_storeu_ps(dst,    yHorner_pack1);
    _mm_storeu_ps(dst+4,  yHorner_pack2);
    _mm_storeu_ps(dst+8,  yHorner_pack3);
    _mm_storeu_ps(dst+12, yHorner_pack4);

    // Update pointers
    impulseResponse0 += 16;
    impulseResponse1 += 16;
    impulseResponse2 += 16;
    impulseResponse3 += 16;
    impulseResponse4 += 16;
    dst              += 16;
  }
}

/**
 *  Update the time variant impulse response, using N=6 prototype impulse responses.
 */
void IasSrcFarrow::updateImpulseResponseN6(float *destination, float tValue) const
{
  float       * __restrict dst = destination;

  float const * __restrict impulseResponse0 = mImpulseResponses[0];
  float const * __restrict impulseResponse1 = mImpulseResponses[1];
  float const * __restrict impulseResponse2 = mImpulseResponses[2];
  float const * __restrict impulseResponse3 = mImpulseResponses[3];
  float const * __restrict impulseResponse4 = mImpulseResponses[4];
  float const * __restrict impulseResponse5 = mImpulseResponses[5];

  __m128 const       tValue_pack = _mm_load1_ps(&tValue);
  __m128 yHorner_pack1;
  __m128 yHorner_pack2;
  __m128 yHorner_pack3;
  __m128 yHorner_pack4;

  for (uint32_t cnt=0; cnt < mFilterLength; cnt+=16)
  {
    yHorner_pack1 = _mm_load_ps(impulseResponse5);
    yHorner_pack2 = _mm_load_ps(impulseResponse5+4);
    yHorner_pack3 = _mm_load_ps(impulseResponse5+8);
    yHorner_pack4 = _mm_load_ps(impulseResponse5+12);

    yHorner_pack1 = _mm_add_ps(_mm_load_ps(impulseResponse4),    _mm_mul_ps(tValue_pack, yHorner_pack1));
    yHorner_pack2 = _mm_add_ps(_mm_load_ps(impulseResponse4+4),  _mm_mul_ps(tValue_pack, yHorner_pack2));
    yHorner_pack3 = _mm_add_ps(_mm_load_ps(impulseResponse4+8),  _mm_mul_ps(tValue_pack, yHorner_pack3));
    yHorner_pack4 = _mm_add_ps(_mm_load_ps(impulseResponse4+12), _mm_mul_ps(tValue_pack, yHorner_pack4));

    yHorner_pack1 = _mm_add_ps(_mm_load_ps(impulseResponse3),    _mm_mul_ps(tValue_pack, yHorner_pack1));
    yHorner_pack2 = _mm_add_ps(_mm_load_ps(impulseResponse3+4),  _mm_mul_ps(tValue_pack, yHorner_pack2));
    yHorner_pack3 = _mm_add_ps(_mm_load_ps(impulseResponse3+8),  _mm_mul_ps(tValue_pack, yHorner_pack3));
    yHorner_pack4 = _mm_add_ps(_mm_load_ps(impulseResponse3+12), _mm_mul_ps(tValue_pack, yHorner_pack4));

    yHorner_pack1 = _mm_add_ps(_mm_load_ps(impulseResponse2),    _mm_mul_ps(tValue_pack, yHorner_pack1));
    yHorner_pack2 = _mm_add_ps(_mm_load_ps(impulseResponse2+4),  _mm_mul_ps(tValue_pack, yHorner_pack2));
    yHorner_pack3 = _mm_add_ps(_mm_load_ps(impulseResponse2+8),  _mm_mul_ps(tValue_pack, yHorner_pack3));
    yHorner_pack4 = _mm_add_ps(_mm_load_ps(impulseResponse2+12), _mm_mul_ps(tValue_pack, yHorner_pack4));

    yHorner_pack1 = _mm_add_ps(_mm_load_ps(impulseResponse1),    _mm_mul_ps(tValue_pack, yHorner_pack1));
    yHorner_pack2 = _mm_add_ps(_mm_load_ps(impulseResponse1+4),  _mm_mul_ps(tValue_pack, yHorner_pack2));
    yHorner_pack3 = _mm_add_ps(_mm_load_ps(impulseResponse1+8),  _mm_mul_ps(tValue_pack, yHorner_pack3));
    yHorner_pack4 = _mm_add_ps(_mm_load_ps(impulseResponse1+12), _mm_mul_ps(tValue_pack, yHorner_pack4));

    yHorner_pack1 = _mm_add_ps(_mm_load_ps(impulseResponse0),    _mm_mul_ps(tValue_pack, yHorner_pack1));
    yHorner_pack2 = _mm_add_ps(_mm_load_ps(impulseResponse0+4),  _mm_mul_ps(tValue_pack, yHorner_pack2));
    yHorner_pack3 = _mm_add_ps(_mm_load_ps(impulseResponse0+8),  _mm_mul_ps(tValue_pack, yHorner_pack3));
    yHorner_pack4 = _mm_add_ps(_mm_load_ps(impulseResponse0+12), _mm_mul_ps(tValue_pack, yHorner_pack4));

    // unaligned store
    _mm_storeu_ps(dst,    yHorner_pack1);
    _mm_storeu_ps(dst+4,  yHorner_pack2);
    _mm_storeu_ps(dst+8,  yHorner_pack3);
    _mm_storeu_ps(dst+12, yHorner_pack4);

    // Update pointers
    impulseResponse0 += 16;
    impulseResponse1 += 16;
    impulseResponse2 += 16;
    impulseResponse3 += 16;
    impulseResponse4 += 16;
    impulseResponse5 += 16;
    dst              += 16;
  }
}

/**
 *  Update the time variant impulse response, using N=7 prototype impulse responses.
 */
void IasSrcFarrow::updateImpulseResponseN7(float *destination, float tValue) const
{
  float       * __restrict dst = destination;

  float const * __restrict impulseResponse0 = mImpulseResponses[0];
  float const * __restrict impulseResponse1 = mImpulseResponses[1];
  float const * __restrict impulseResponse2 = mImpulseResponses[2];
  float const * __restrict impulseResponse3 = mImpulseResponses[3];
  float const * __restrict impulseResponse4 = mImpulseResponses[4];
  float const * __restrict impulseResponse5 = mImpulseResponses[5];
  float const * __restrict impulseResponse6 = mImpulseResponses[6];

  __m128 const       tValue_pack = _mm_load1_ps(&tValue);
  __m128 yHorner_pack1;
  __m128 yHorner_pack2;
  __m128 yHorner_pack3;
  __m128 yHorner_pack4;

  for (uint32_t cnt=0; cnt < mFilterLength; cnt+=16)
  {
    yHorner_pack1 = _mm_load_ps(impulseResponse6);
    yHorner_pack2 = _mm_load_ps(impulseResponse6+4);
    yHorner_pack3 = _mm_load_ps(impulseResponse6+8);
    yHorner_pack4 = _mm_load_ps(impulseResponse6+12);

    yHorner_pack1 = _mm_add_ps(_mm_load_ps(impulseResponse5),    _mm_mul_ps(tValue_pack, yHorner_pack1));
    yHorner_pack2 = _mm_add_ps(_mm_load_ps(impulseResponse5+4),  _mm_mul_ps(tValue_pack, yHorner_pack2));
    yHorner_pack3 = _mm_add_ps(_mm_load_ps(impulseResponse5+8),  _mm_mul_ps(tValue_pack, yHorner_pack3));
    yHorner_pack4 = _mm_add_ps(_mm_load_ps(impulseResponse5+12), _mm_mul_ps(tValue_pack, yHorner_pack4));

    yHorner_pack1 = _mm_add_ps(_mm_load_ps(impulseResponse4),    _mm_mul_ps(tValue_pack, yHorner_pack1));
    yHorner_pack2 = _mm_add_ps(_mm_load_ps(impulseResponse4+4),  _mm_mul_ps(tValue_pack, yHorner_pack2));
    yHorner_pack3 = _mm_add_ps(_mm_load_ps(impulseResponse4+8),  _mm_mul_ps(tValue_pack, yHorner_pack3));
    yHorner_pack4 = _mm_add_ps(_mm_load_ps(impulseResponse4+12), _mm_mul_ps(tValue_pack, yHorner_pack4));

    yHorner_pack1 = _mm_add_ps(_mm_load_ps(impulseResponse3),    _mm_mul_ps(tValue_pack, yHorner_pack1));
    yHorner_pack2 = _mm_add_ps(_mm_load_ps(impulseResponse3+4),  _mm_mul_ps(tValue_pack, yHorner_pack2));
    yHorner_pack3 = _mm_add_ps(_mm_load_ps(impulseResponse3+8),  _mm_mul_ps(tValue_pack, yHorner_pack3));
    yHorner_pack4 = _mm_add_ps(_mm_load_ps(impulseResponse3+12), _mm_mul_ps(tValue_pack, yHorner_pack4));

    yHorner_pack1 = _mm_add_ps(_mm_load_ps(impulseResponse2),    _mm_mul_ps(tValue_pack, yHorner_pack1));
    yHorner_pack2 = _mm_add_ps(_mm_load_ps(impulseResponse2+4),  _mm_mul_ps(tValue_pack, yHorner_pack2));
    yHorner_pack3 = _mm_add_ps(_mm_load_ps(impulseResponse2+8),  _mm_mul_ps(tValue_pack, yHorner_pack3));
    yHorner_pack4 = _mm_add_ps(_mm_load_ps(impulseResponse2+12), _mm_mul_ps(tValue_pack, yHorner_pack4));

    yHorner_pack1 = _mm_add_ps(_mm_load_ps(impulseResponse1),    _mm_mul_ps(tValue_pack, yHorner_pack1));
    yHorner_pack2 = _mm_add_ps(_mm_load_ps(impulseResponse1+4),  _mm_mul_ps(tValue_pack, yHorner_pack2));
    yHorner_pack3 = _mm_add_ps(_mm_load_ps(impulseResponse1+8),  _mm_mul_ps(tValue_pack, yHorner_pack3));
    yHorner_pack4 = _mm_add_ps(_mm_load_ps(impulseResponse1+12), _mm_mul_ps(tValue_pack, yHorner_pack4));

    yHorner_pack1 = _mm_add_ps(_mm_load_ps(impulseResponse0),    _mm_mul_ps(tValue_pack, yHorner_pack1));
    yHorner_pack2 = _mm_add_ps(_mm_load_ps(impulseResponse0+4),  _mm_mul_ps(tValue_pack, yHorner_pack2));
    yHorner_pack3 = _mm_add_ps(_mm_load_ps(impulseResponse0+8),  _mm_mul_ps(tValue_pack, yHorner_pack3));
    yHorner_pack4 = _mm_add_ps(_mm_load_ps(impulseResponse0+12), _mm_mul_ps(tValue_pack, yHorner_pack4));

    // unaligned store
    _mm_storeu_ps(dst,    yHorner_pack1);
    _mm_storeu_ps(dst+4,  yHorner_pack2);
    _mm_storeu_ps(dst+8,  yHorner_pack3);
    _mm_storeu_ps(dst+12, yHorner_pack4);

    // Update pointers
    impulseResponse0 += 16;
    impulseResponse1 += 16;
    impulseResponse2 += 16;
    impulseResponse3 += 16;
    impulseResponse4 += 16;
    impulseResponse5 += 16;
    impulseResponse6 += 16;
    dst              += 16;
  }
}

#endif // IASSRCFARROWCONFIG_USE_SSE


/*
 * Function to get a IasSrcFarrow::IasResult as string.
 */
#define STRING_RETURN_CASE(name) case name: return std::string(#name); break
#define DEFAULT_STRING(name) default: return std::string(name)
std::string toString(const IasSrcFarrow::IasResult& type)
{
  switch(type)
  {
    STRING_RETURN_CASE(IasSrcFarrow::eIasOk);
    STRING_RETURN_CASE(IasSrcFarrow::eIasInvalidParam);
    STRING_RETURN_CASE(IasSrcFarrow::eIasInitFailed);
    STRING_RETURN_CASE(IasSrcFarrow::eIasNotInitialized);
    STRING_RETURN_CASE(IasSrcFarrow::eIasFailed);
    DEFAULT_STRING("Invalid IasSrcFarrow::IasResult => " + std::to_string(type));
  }
}


} // namespace IasAudio
