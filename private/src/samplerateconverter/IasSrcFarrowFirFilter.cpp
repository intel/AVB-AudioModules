/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file    IasSrcFarrowFirFilter.cpp
 * @brief   FIR filter to be used within the sample rate converter
 *          based on Farrow's structure.
 * @date    2015
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "samplerateconverter/IasSrcFarrowFirFilter.hpp"


#if IASSRCFARROWCONFIG_USE_SSE
#include <xmmintrin.h>
#include <emmintrin.h>
#endif

#ifdef __linux__
#define MS_VC  0
#else
#define MS_VC  1
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

/*
 * Constant factors for conversion from Int16 and Int32 to Float32.
 */
static const float cConversionFactor_int2float   = 1.0f / static_cast<float>(0x7fffffff);
static const float cConversionFactor_short2float = 1.0f / static_cast<float>(0x7fff);

/*
 * Constant factors for conversion from Float32 to Int16 and Int32. These factors include the output gain.
 */
static const float cOutputGain                   = IASSRCFARROWCONFIG_OUTPUT_GAIN;
static const float cConversionFactor_float2int   = static_cast<float>(0x7fffffff) * cOutputGain;
static const float cConversionFactor_float2short = static_cast<float>(0x7fff)     * cOutputGain;

/*
 * Maximum and minimum values of integers in floating point representation.
 */
static const float cIntMaxVal   =  2147483647.0f; // 0x7fffffff
static const float cIntMinVal   = -2147483648.0f; // 0x80000000
static const float cShortMaxVal =       32767.0f; //     0x7fff
static const float cShortMinVal =      -32768.0f; //     0x8000


/**
 * Conversion from the input format (either float or int32_t or int16_t) into float.
 */
inline float convert2Float(float a)
{
  return a;
}

inline float convert2Float(int32_t a)
{
  return (static_cast<float>(a) * cConversionFactor_int2float);
}

inline float convert2Float(int16_t a)
{
  return (static_cast<float>(a) * cConversionFactor_short2float);
}


/**
 * Increase the index of a ring buffer, using circular addressing.
 */
inline uint32_t  increaseIndex(uint32_t index, uint32_t ringBufferLength)
{
  index++;
  if (index >= ringBufferLength)
  {
    index = 0;
  }
  return index;
}


/**
 * Decrease the index of a ring buffer, using circular addressing.
 */
inline uint32_t  decreaseIndex(uint32_t index, uint32_t ringBufferLength)
{
  if (index > 0)
  {
    return index-1;
  }
  else
  {
    return ringBufferLength-1;
  }
}


#if !(IASSRCFARROWCONFIG_USE_SSE)

/**
 * Apply the output gain and convert from float to the output format.
 * This is the variant for output format float. No saturation is applied.
 */
inline void convertFloat2Output(float *output, const float &input)
{
  *output = input * cOutputGain;
}

/**
 * Apply the output gain and convert from float to the output format.
 * This is the variant for output format int32_t.
 * The result is saturated if the compiler flag IASSRCFARROWCONFIG_USE_SATURATION is set.
 */
inline void convertFloat2Output(int32_t *output, const float &input)
{
  float a = input * cConversionFactor_float2int;

#if (IASSRCFARROWCONFIG_USE_SATURATION)
  if (a > cIntMaxVal)
  {
    *output = 0x7fffffff;
  }
  else if (a < cIntMinVal)
  {
    *output = 0x80000000;
  }
  else
  {
    *output = static_cast<int32_t>(a);
  }
#else
  *output = static_cast<int32_t>(a);
#endif
}


/**
 * Apply the output gain and convert from float to the output format.
 * This is the variant for output format int16_t.
 * The result is saturated if the compiler flag IASSRCFARROWCONFIG_USE_SATURATION is set.
 */
inline void convertFloat2Output(int16_t *output, const float &input)
{
  float a = input * cConversionFactor_float2short;

#if (IASSRCFARROWCONFIG_USE_SATURATION)
  if (a > cShortMaxVal)
  {
    *output = static_cast<int16_t>(0x7fff);
  }
  else if (a < cShortMinVal)
  {
    *output = static_cast<int16_t>(0x8000);
  }
  else
  {
    *output = static_cast<int16_t>(a);
  }
#else
  *output = static_cast<int16_t>(a);
#endif
}


#else // now following... the conversion functions for the SSE variants.

/**
 * Apply output gain and store the lowest single-precision floating-point value
 * of a __m128 register to a float* memory location.
 */
inline void storeLowValue(float *p, __m128 a)
{
  static const __m128  cOutputGain_mm =  _mm_load1_ps(&cOutputGain);
  a = _mm_mul_ps(a, cOutputGain_mm); // apply output gain
  _mm_store_ss(p, a); // store the lowest single-precision floating-point value
}


/**
 * Apply output gain and store the lowest single-precision floating-point value
 * of a __m128 register to a int32_t* memory location. This function does a
 * conversion from float to int32_t.
 */
inline void storeLowValue(int32_t *p, __m128 a)
{
  static const __m128  cConversionFactor_mm  = _mm_load1_ps(&cConversionFactor_float2int);
  float b;
  a  = _mm_mul_ps(a, cConversionFactor_mm); // multiply with 2^31-1 and apply output gain
  _mm_store_ss(&b, a); // store the lowest single-precision floating-point value

#if IASSRCFARROWCONFIG_USE_SATURATION
  if (b > cIntMaxVal)
  {
    *p = 0x7fffffff;
  }
  else if (b < cIntMinVal)
  {
    *p = 0x80000000;
  }
  else
  {
    *p = static_cast<int32_t>(b);
  }
#else
  *p = static_cast<int32_t>(b);
#endif
}


/**
 * Apply output gain and store the lowest single-precision floating-point value
 * of a __m128 register to a int16_t* memory location. This function does a
 * conversion from float to int16_t.
 */
inline void storeLowValue(int16_t *p, __m128 a)
{
  static const __m128  cConversionFactor_mm  = _mm_load1_ps(&cConversionFactor_float2short);
  __m128i    b;
  int32_t intValue;

  a        = _mm_mul_ps(a, cConversionFactor_mm); // multiply with 2^15-1 and apply output gain
  b        = _mm_cvtps_epi32(a);                  // convert from Float32 to Int32
#if IASSRCFARROWCONFIG_USE_SATURATION
  b        = _mm_packs_epi32(b,b);                // convert Int32 to Int16 with saturation
#endif
  intValue = _mm_cvtsi128_si32(b);                // move the lowest 32 bit to scalar intValue
  *p       = static_cast<int16_t>(intValue);
}

#endif




/*****************************************************************************
 * @brief Constructor
 *****************************************************************************
 */
IasSrcFarrowFirFilter::IasSrcFarrowFirFilter()
  :mImpulseResponses(NULL)
  ,mRingBuffers(NULL)
  ,mRingBufferIndex(0)
#if IASSRCFARROWCONFIG_USE_SSE
  ,mPaddingForSSE((mRingBufferIndex+1) & 0x00000003)
#endif
  ,mMaxFilterLength(0)
  ,mFilterLength(0)
  ,mNumImpulseResponses(0)
  ,mMaxNumInputChannels(0)
  ,mIsInitialized(false)
{
}


/*****************************************************************************
 * @brief Destructor
 *****************************************************************************
 */
IasSrcFarrowFirFilter::~IasSrcFarrowFirFilter()
{
  uint32_t chan;

  if (mIsInitialized)
  {
#if MS_VC
    // Deallocate the ring buffers
    for (chan=0; chan<mMaxNumInputChannels; chan++)
    {
      _aligned_free(mRingBuffers[chan]);
      mRingBuffers[chan] = NULL;
    }
    _aligned_free(mRingBuffers);
    _aligned_free(mImpulseResponses);
#else
    // Deallocate the ring buffers
    for (chan=0; chan<mMaxNumInputChannels; chan++)
    {
      free(mRingBuffers[chan]);
      mRingBuffers[chan] = NULL;
    }
    free(mRingBuffers);
    free(mImpulseResponses);
#endif

    mRingBuffers = NULL;
    mImpulseResponses = NULL;
  }
}


/*****************************************************************************
 * @brief Init function
 *****************************************************************************
 */
int IasSrcFarrowFirFilter::init(uint32_t  maxFilterLength,
                                uint32_t  numImpulseResponses,
                                uint32_t  maxNumInputChannels)
{
  uint32_t chan;

  if (mIsInitialized)
  {
    return 1;
  }
  if (maxFilterLength < 2)
  {
    return 1;
  }

  // Allocate the vector of pointers to the impulse responses
#if MS_VC
  mImpulseResponses = (const float**)_aligned_malloc(numImpulseResponses*sizeof(float*), 16);
#else
   mImpulseResponses = (const float**)memalign(16, numImpulseResponses*sizeof(float*));
#endif
  if (mImpulseResponses == NULL)
  {
    return 1;
  }

  // Allocate the vector of pointers to the ring buffers
#if MS_VC
  mRingBuffers = (float**)_aligned_malloc(maxNumInputChannels*sizeof(float*), 16);
#else
  mRingBuffers = (float**)memalign(16, maxNumInputChannels*sizeof(float*));
#endif
  if (mRingBuffers == NULL)
  {
    return 1;
  }

  // Allocate the ring buffers
  for (chan=0; chan<maxNumInputChannels; chan++)
  {
#if MS_VC
    mRingBuffers[chan] = (float*)_aligned_malloc(2*maxFilterLength*sizeof(float), 16);
#else
    mRingBuffers[chan] = (float*)memalign(16, 2*maxFilterLength*sizeof(float));
#endif
    if (mRingBuffers[chan] == NULL)
    {
      return 1;
    }
  }

  mNumImpulseResponses = numImpulseResponses;
  mMaxNumInputChannels = maxNumInputChannels;
  mMaxFilterLength     = maxFilterLength;
  mFilterLength        = 0;
  mIsInitialized       = true;

#if IASSRCFARROWCONFIG_USE_SSE
  mPaddingForSSE = (mRingBufferIndex+1) & 0x00000003;
#endif

  // Call the reset function
  this->reset();

  return 0;
}

/*****************************************************************************
 * @brief Set the filter length.
 *****************************************************************************
 */
int IasSrcFarrowFirFilter::setFilterLength(uint32_t filterLength)
{
  if (filterLength > mMaxFilterLength)
  {
   return 1;
  }

  mFilterLength = filterLength;

  return 0;
}


/*****************************************************************************
 * @brief Set the impulse response.
 *****************************************************************************
 */
int IasSrcFarrowFirFilter::setImpulseResponse(float const **impulseResponses,
                                              uint32_t          filterLength)
{
  uint32_t chan;

  if ((impulseResponses == NULL) || (filterLength > mMaxFilterLength))
  {
    return 1;
  }

  mFilterLength = filterLength;

  for (chan=0; chan<mNumImpulseResponses; chan++)
  {
    if (impulseResponses[chan] == NULL)
    {
      return 1;
    }

    mImpulseResponses[chan] = impulseResponses[chan];
  }

  return 0;
}

/*****************************************************************************
 * @brief Reset function.
 *****************************************************************************
 */
int IasSrcFarrowFirFilter::reset()
{
  uint32_t chan;

  if (!mIsInitialized)
  {
    return 1;
  }
  mRingBufferIndex = 0;

  for (chan=0; chan<mMaxNumInputChannels; chan++)
  {
    if (mRingBuffers[chan] == NULL)
    {
      return 1;
    }

    memset(mRingBuffers[chan], 0, 2*mMaxFilterLength*sizeof(mRingBuffers[0][0]));
  }
  return 0;
}

/*****************************************************************************
 * @brief Insert a new input sample into the internal ring buffer.
 *****************************************************************************
 */
int IasSrcFarrowFirFilter::singleInputInsertSample(float const *inputBuffer,
                                                   uint32_t         inputBufferIndex)
{
  // Verify that mRingBufferIndex is valid. In practice, the assert condition
  // will never fail, because mRingBufferIndex is under control of this component.
  IAS_ASSERT(mRingBufferIndex < mFilterLength);

  // Insert the current sample into the ring buffer. We need only one
  // channel (channel 0) for this function.
  mRingBuffers[0][mRingBufferIndex]               = inputBuffer[inputBufferIndex];
  mRingBuffers[0][mRingBufferIndex+mFilterLength] = inputBuffer[inputBufferIndex];

  // Decrease the buffer write index. The ring buffer is organized such that the
  // input samples are written from the right to the left.
  mRingBufferIndex = decreaseIndex(mRingBufferIndex, mFilterLength);

  return 0;
}

/*****************************************************************************
 * @brief Process the FIR filter for one sample: this function filters
 *        one single input channel by N different impulse responses.
 *        This results in N different output signals.
 *****************************************************************************
 */
int IasSrcFarrowFirFilter::singleInputProcessSample(float *outputSamples)
{
  uint32_t  chan;
  uint32_t  cnt;
  uint32_t  index2;
  float sum;

  for (chan=0; chan < mNumImpulseResponses; chan++)
  {
    index2 = increaseIndex(mRingBufferIndex, mFilterLength);
    sum    = 0.0f;

    // Calculate the convolution sum.
    for (cnt=0; cnt < mFilterLength; cnt++)
    {
      sum = sum + mRingBuffers[0][index2] * mImpulseResponses[chan][cnt];
      index2++;
    }
    outputSamples[chan] = sum;
  }

  return 0;
}

/*****************************************************************************
 * @brief Insert a new input sample (of M input channels) into the internal
 *        ring buffers.
 *****************************************************************************
 */
template <typename T>
int IasSrcFarrowFirFilter::multiInputInsertSample(T            const **inputBuffers,
                                                  uint32_t          inputBufferIndex,
                                                  uint32_t          numInputChannels)
{
  uint32_t  chan;
  float inputSample;  // sample from inputBuffers, after conversion into float.

  if ((mNumImpulseResponses != 1) || (numInputChannels > mMaxNumInputChannels))
  {
    return 1;
  }

  for (chan=0; chan<numInputChannels; chan++)
  {
    // Insert the current sample of all input channels into the ring buffers.
    inputSample = convert2Float(inputBuffers[chan][inputBufferIndex]);
    mRingBuffers[chan][mRingBufferIndex]               = inputSample;
    mRingBuffers[chan][mRingBufferIndex+mFilterLength] = inputSample;
  }

  // Decrease the buffer write index. The ring buffer is organized such that the
  // input samples are written from the right to the left.
  mRingBufferIndex = decreaseIndex(mRingBufferIndex, mFilterLength);

#if IASSRCFARROWCONFIG_USE_SSE
  mPaddingForSSE = (mRingBufferIndex+1) & 0x00000003;
#endif

  return 0;
}

/*
 * Tell the compiler that we need this template/function for float, int32_t, and int16_t
 */

template int IasSrcFarrowFirFilter::multiInputInsertSample<float>(float const **inputBuffers,
                                                                         uint32_t          inputBufferIndex,
                                                                         uint32_t          numInputChannels);

template int IasSrcFarrowFirFilter::multiInputInsertSample<int32_t>(int32_t   const **inputBuffers,
                                                                       uint32_t          inputBufferIndex,
                                                                       uint32_t          numInputChannels);

template int IasSrcFarrowFirFilter::multiInputInsertSample<int16_t>(int16_t   const **inputBuffers,
                                                                       uint32_t          inputBufferIndex,
                                                                       uint32_t          numInputChannels);


#if !(IASSRCFARROWCONFIG_USE_SSE)  // The normal variant (without SSE optimization)

/*****************************************************************************
 * @brief Process the FIR filter for one sample: this function filters
 *        M individual input channels by one single impulse response,
 *        which is jointly used for all channels.
 *****************************************************************************
 */
template <typename T>
int IasSrcFarrowFirFilter::multiInputProcessSample(T            **outputBuffers,
                                                   uint32_t    outputBufferIndex,
                                                   uint32_t    numChannels)
{
  uint32_t  chan;
  uint32_t  cnt;
  uint32_t  index2;
  float sum;

  if ((mNumImpulseResponses != 1) || (numChannels > mMaxNumInputChannels))
  {
    return 1;
  }

  for (chan=0; chan<numChannels; chan++)
  {
    index2 = increaseIndex(mRingBufferIndex, mFilterLength);
    sum    = 0.0f;

    // Calculate the convolution sum.
    for (cnt=0; cnt<mFilterLength; cnt++)
    {
      sum = sum + mRingBuffers[chan][index2] * mImpulseResponses[0][cnt];
      index2++;
    }
    // Write sum into output buffer. Do a conversion from float to integer, if required.
    convertFloat2Output(&outputBuffers[chan][outputBufferIndex], sum);
  }

  return 0;
}

#else // !(IASSRCFARROWCONFIG_USE_SSE) // Now following... the SSE-optimized variant

template <typename T>
int IasSrcFarrowFirFilter::multiInputProcessSample(T            **outputBuffers,
                                                   uint32_t    outputBufferIndex,
                                                   uint32_t    numChannels)
{
  uint32_t  chan;
  uint32_t  cnt;
  uint32_t  index2;

  __m128i shift32 = _mm_set_epi32(0, 0, 0, 32);

  if ((mNumImpulseResponses != 1) || (numChannels > mMaxNumInputChannels) || (numChannels < 1))
  {
    return 1;
  }

  __m128       impulse_responses_pack;
  __m128       ringbuffer_pack1, ringbuffer_pack2;
  __m128       ac0, ac1;

  // Determine loop number based on if have any padding
  uint32_t loop_num = (mPaddingForSSE == 0) ? mFilterLength/4 : mFilterLength/4+1;

  // Calcute 2 channels in each iteration.
  // This loop is executed floor(numChannels/2) times.
  for (chan=0; chan < numChannels-1; chan+=2)
  {
    // round index2 to make mRingBuffers[chan][index2] 16-bytes aligned
    index2 = increaseIndex(mRingBufferIndex, mFilterLength) - mPaddingForSSE;
    ac0 = _mm_setzero_ps();
    ac1 = _mm_setzero_ps();

    // Calculate the convolution sum.
#if INTEL_COMPILER
    // Let the Intel compiler unroll the following loop by a factor of 4.
    #pragma unroll(4)
#endif
    for (cnt=0; cnt<loop_num; cnt++)
    {
      // sum = sum + mRingBuffers[chan][index2] * mImpulseResponses[0][cnt];
      impulse_responses_pack = _mm_load_ps(&mImpulseResponses[0][cnt*4]);
      ringbuffer_pack1 = _mm_load_ps(&(mRingBuffers[chan][index2]));
      ringbuffer_pack2 = _mm_load_ps(&(mRingBuffers[chan+1][index2]));
      ac0 = _mm_add_ps(ac0, _mm_mul_ps(ringbuffer_pack1, impulse_responses_pack));
      ac1 = _mm_add_ps(ac1, _mm_mul_ps(ringbuffer_pack2, impulse_responses_pack));
      index2 += 4;
    }
    ac0 = _mm_add_ps(
                     _mm_unpacklo_ps(ac0, ac1),
                     _mm_unpackhi_ps(ac0, ac1)
                     );
    ac0 = _mm_add_ps(ac0, _mm_movehl_ps(ac0, ac0));
    storeLowValue(&outputBuffers[chan][outputBufferIndex], ac0);                    // output chan
    storeLowValue(&outputBuffers[chan+1][outputBufferIndex],
                  _mm_castsi128_ps(_mm_srl_epi64(_mm_castps_si128(ac0), shift32))); // output chan+1
  }

  // If numChannels is odd, process the last channel individually.
  if ((numChannels & 0x01) != 0)
  {
    chan = numChannels-1;

    // round index2 to make mRingBuffers[chan][index2] 16-bytes aligned
    index2 = increaseIndex(mRingBufferIndex, mFilterLength) - mPaddingForSSE;
    ac0 = _mm_setzero_ps();

    //#pragma ivdep
    //   #pragma vector aligned
    //   #pragma unroll(8)
    // Calculate the convolution sum.
    for (cnt=0; cnt<loop_num; cnt++)
    {
      // sum = sum + mRingBuffers[chan][index2] * mImpulseResponses[0][cnt];
      impulse_responses_pack = _mm_load_ps(&mImpulseResponses[0][cnt*4]);
      ringbuffer_pack1 = _mm_load_ps(&(mRingBuffers[chan][index2]));
      ac0 = _mm_add_ps(ac0, _mm_mul_ps(ringbuffer_pack1, impulse_responses_pack));
      index2 += 4;
    }

    // accumulate partial sums
    // movhl   [s3, s2, s1, s0] [s3, s2, s1, s0] => [s3, s2, s1, s0] [s3, s2, s3, s2]
    // add     [s3, s2, s1, s0] [s3, s2, s3, s2] => [s3, s2, s1+s3, s0+s2]
    // shuffle [s3, s2, s1+s3, s0+s2] [s3, s2, s1+s3, s0+s2] 1 => [x, x, x, s1+s3]
    // add     [s3, s2, s1+s3, s0+s2] [x, x, x, s1+s3] => [x, x, x, s0+s1+s2+s3]
    __m128 tmp = _mm_add_ps(ac0, _mm_movehl_ps(ac0, ac0));
    __m128 sum = _mm_add_ss(tmp, _mm_shuffle_ps(tmp, tmp, 1));
    storeLowValue(&outputBuffers[chan][outputBufferIndex], sum);   // output
  }

  return 0;
}

#endif // #if IASSRCFARROWCONFIG_USE_SSE

/*
 * Tell the compiler that we need this template/function for float, int32_t, and int16_t
 */

template int IasSrcFarrowFirFilter::multiInputProcessSample<float>(float **outputBuffers,
                                                                          uint32_t    outputBufferIndex,
                                                                          uint32_t    numChannels);

template int IasSrcFarrowFirFilter::multiInputProcessSample<int32_t>(int32_t   **outputBuffers,
                                                                        uint32_t    outputBufferIndex,
                                                                        uint32_t    numChannels);

template int IasSrcFarrowFirFilter::multiInputProcessSample<int16_t>(int16_t   **outputBuffers,
                                                                        uint32_t    outputBufferIndex,
                                                                        uint32_t    numChannels);

} // namespace IasAudio
