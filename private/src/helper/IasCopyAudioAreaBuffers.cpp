/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasCopyAudioAreaBuffers.cpp
 * @date   2015
 * @brief
 */

#include <cmath>
#include <limits>

#include "audio/common/IasAudioCommonTypes.hpp"
#include "internal/audio/common/helper/IasCopyAudioAreaBuffers.hpp"

#include <xmmintrin.h>
#include <emmintrin.h>

namespace IasAudio {

static const int32_t   cInt32Min = std::numeric_limits<int32_t>::min();
static const int32_t   cInt32Max = std::numeric_limits<int32_t>::max();
static const float cFactorFloat32toInt16    = 32768.0f;
static const float cFactorFloat32toInt32    = 2147483647.0f;
static const float cFactorInt16toFloat32    = 1.0f / 32768.0f;
static const float cFactorInt32toFloat32    = 1.0f / 2147483648.0f;

static const __m128       cFactorFloat32toInt16_mm = _mm_load1_ps(&cFactorFloat32toInt16);


// Efficient function for copying one channel with non-interleaved 16 bit samples without format conversion.
// If destinNumSamples is greater than sourceNumSamples, zero-valued samples will be padded.
static void copyNonInterleavedChannel16bit(uint8_t *destinPtr, uint32_t destinStep, uint32_t destinNumSamples,
                                           uint8_t *sourcePtr, uint32_t sourceStep, uint32_t sourceNumSamples)
{
  (void)destinStep;
  (void)sourceStep;
  const uint32_t sampleSize = 2u; // 2 byte = 16 bit
  IasAudioCommonResult result = ias_safe_memcpy(destinPtr, destinNumSamples * sampleSize, sourcePtr, sourceNumSamples * sampleSize);
  IAS_ASSERT(eIasResultOk == result);
  IAS_ASSERT(destinPtr != nullptr);
  (void)result;
  if (destinNumSamples > sourceNumSamples)
  {
    // Fill the remaining part of the destination buffer with zeros.
    memset(destinPtr + sourceNumSamples * sampleSize, 0, (destinNumSamples - sourceNumSamples) * sampleSize);
  }
}


// Efficient function for copying one channel with non-interleaved 32 bit samples without format conversion.
// If destinNumSamples is greater than sourceNumSamples, zero-valued samples will be padded.
static void copyNonInterleavedChannel32bit(uint8_t *destinPtr, uint32_t destinStep, uint32_t destinNumSamples,
                                           uint8_t *sourcePtr, uint32_t sourceStep, uint32_t sourceNumSamples)
{
  (void)destinStep;
  (void)sourceStep;
  const uint32_t sampleSize = 4u; // 4 byte = 32 bit
  IasAudioCommonResult result = ias_safe_memcpy(destinPtr, destinNumSamples * sampleSize, sourcePtr, sourceNumSamples * sampleSize);
  IAS_ASSERT(eIasResultOk == result);
  IAS_ASSERT(destinPtr != nullptr);
  (void)result;
  if (destinNumSamples > sourceNumSamples)
  {
    // Fill the remaining part of the destination buffer with zeros.
    memset(destinPtr + sourceNumSamples * sampleSize, 0, (destinNumSamples - sourceNumSamples) * sampleSize);
  }
}


static void copyChannelInt16toInt16(uint8_t *destinPtr, uint32_t destinStep, uint32_t destinNumSamples,
                                    uint8_t *sourcePtr, uint32_t sourceStep, uint32_t sourceNumSamples)
{
  int16_t * __restrict destinSample = (int16_t*)destinPtr;
  int16_t * __restrict sourceSample = (int16_t*)sourcePtr;
  uint32_t minSamples = std::min(destinNumSamples, sourceNumSamples);
  destinStep = destinStep >> 1;
  sourceStep = sourceStep >> 1;
  for (uint32_t cntSamples = 0; cntSamples < minSamples; cntSamples++)
  {
    *destinSample = *sourceSample;
    destinSample += destinStep;
    sourceSample += sourceStep;
  }
  for (uint32_t cntSamples = minSamples; cntSamples < destinNumSamples; cntSamples++)
  {
    *destinSample = 0;
    destinSample += destinStep;
  }
}


static void copyChannelInt16toInt32(uint8_t *destinPtr, uint32_t destinStep, uint32_t destinNumSamples,
                                    uint8_t *sourcePtr, uint32_t sourceStep, uint32_t sourceNumSamples)
{
  int32_t * __restrict destinSample = (int32_t*)destinPtr;
  int16_t * __restrict sourceSample = (int16_t*)sourcePtr;
  uint32_t minSamples = std::min(destinNumSamples, sourceNumSamples);
  destinStep = destinStep >> 2;
  sourceStep = sourceStep >> 1;
  for (uint32_t cntSamples = 0; cntSamples < minSamples; cntSamples++)
  {
    *destinSample = static_cast<int32_t>(*sourceSample) << 16;
    destinSample += destinStep;
    sourceSample += sourceStep;
  }
  for (uint32_t cntSamples = minSamples; cntSamples < destinNumSamples; cntSamples++)
  {
    *destinSample = 0;
    destinSample += destinStep;
  }
}


static void copyChannelInt16toFloat32(uint8_t *destinPtr, uint32_t destinStep, uint32_t destinNumSamples,
                                      uint8_t *sourcePtr, uint32_t sourceStep, uint32_t sourceNumSamples)
{
  float * __restrict destinSample = (float*)destinPtr;
  int16_t   * __restrict sourceSample = (int16_t*)sourcePtr;
  uint32_t minSamples = std::min(destinNumSamples, sourceNumSamples);
  destinStep = destinStep >> 2;
  sourceStep = sourceStep >> 1;
  for (uint32_t cntSamples = 0; cntSamples < minSamples; cntSamples++)
  {
    *destinSample = static_cast<float>(*sourceSample) * cFactorInt16toFloat32;
    destinSample += destinStep;
    sourceSample += sourceStep;
  }
  for (uint32_t cntSamples = minSamples; cntSamples < destinNumSamples; cntSamples++)
  {
    *destinSample = 0.0f;
    destinSample += destinStep;
  }
}


static void copyChannelInt32toInt16(uint8_t *destinPtr, uint32_t destinStep, uint32_t destinNumSamples,
                                    uint8_t *sourcePtr, uint32_t sourceStep, uint32_t sourceNumSamples)
{
  int16_t * __restrict destinSample = (int16_t*)destinPtr;
  int32_t * __restrict sourceSample = (int32_t*)sourcePtr;
  uint32_t minSamples = std::min(destinNumSamples, sourceNumSamples);
  destinStep = destinStep >> 1;
  sourceStep = sourceStep >> 2;
  for (uint32_t cntSamples = 0; cntSamples < minSamples; cntSamples++)
  {
    // Skip the lower 16 bit with rounding
    *destinSample = static_cast<int16_t>(((*sourceSample >> 15) + 1) >> 1);
    destinSample += destinStep;
    sourceSample += sourceStep;
  }
  for (uint32_t cntSamples = minSamples; cntSamples < destinNumSamples; cntSamples++)
  {
    *destinSample = 0;
    destinSample += destinStep;
  }
}


static void copyChannelInt32toInt32(uint8_t *destinPtr, uint32_t destinStep, uint32_t destinNumSamples,
                                    uint8_t *sourcePtr, uint32_t sourceStep, uint32_t sourceNumSamples)
{
  int32_t * __restrict destinSample = (int32_t*)destinPtr;
  int32_t * __restrict sourceSample = (int32_t*)sourcePtr;
  uint32_t minSamples = std::min(destinNumSamples, sourceNumSamples);
  destinStep = destinStep >> 2;
  sourceStep = sourceStep >> 2;
  for (uint32_t cntSamples = 0; cntSamples < minSamples; cntSamples++)
  {
    *destinSample = *sourceSample;
    destinSample += destinStep;
    sourceSample += sourceStep;
  }
  for (uint32_t cntSamples = minSamples; cntSamples < destinNumSamples; cntSamples++)
  {
    *destinSample = 0;
    destinSample += destinStep;
  }
}


static void copyChannelInt32toFloat32(uint8_t *destinPtr, uint32_t destinStep, uint32_t destinNumSamples,
                                      uint8_t *sourcePtr, uint32_t sourceStep, uint32_t sourceNumSamples)
{
  float * __restrict destinSample = (float*)destinPtr;
  int32_t   * __restrict sourceSample = (int32_t*)sourcePtr;
  uint32_t minSamples = std::min(destinNumSamples, sourceNumSamples);
  destinStep = destinStep >> 2;
  sourceStep = sourceStep >> 2;
  for (uint32_t cntSamples = 0; cntSamples < minSamples; cntSamples++)
  {
    *destinSample = static_cast<float>(*sourceSample) * cFactorInt32toFloat32;
    destinSample += destinStep;
    sourceSample += sourceStep;
  }
  for (uint32_t cntSamples = minSamples; cntSamples < destinNumSamples; cntSamples++)
  {
    *destinSample = 0.0f;
    destinSample += destinStep;
  }
}


static void copyChannelFloat32toInt16(uint8_t *destinPtr, uint32_t destinStep, uint32_t destinNumSamples,
                                      uint8_t *sourcePtr, uint32_t sourceStep, uint32_t sourceNumSamples)
{
  int16_t   * __restrict destinSample = (int16_t*)destinPtr;
  float * __restrict sourceSample = (float*)sourcePtr;
  uint32_t minSamples = std::min(destinNumSamples, sourceNumSamples);
  destinStep = destinStep >> 1;
  sourceStep = sourceStep >> 2;
  for (uint32_t cntSamples = 0; cntSamples < minSamples; cntSamples++)
  {
    __m128     a;
    __m128i    b;
    int32_t int32Value;
    int16_t int16Value;
    a = _mm_load1_ps(sourceSample);
    a = _mm_mul_ps(a, cFactorFloat32toInt16_mm);      // multiply with 32768.0f
    b = _mm_cvtps_epi32(a);                           // convert from Float32 to Int32 according to rounding mode
    b = _mm_packs_epi32(b, b);                        // convert Int32 to Int16 with saturation
    int32Value = _mm_cvtsi128_si32(b);                // move the lowest 32 bit to scalar intValue
    int16Value = static_cast<int16_t>(int32Value); // use only lowest 16 bit fro output value

    *destinSample = int16Value;

    destinSample += destinStep;
    sourceSample += sourceStep;
  }
  for (uint32_t cntSamples = minSamples; cntSamples < destinNumSamples; cntSamples++)
  {
    *destinSample = 0;
    destinSample += destinStep;
  }
}


static void copyChannelFloat32toInt32(uint8_t *destinPtr, uint32_t destinStep, uint32_t destinNumSamples,
                                      uint8_t *sourcePtr, uint32_t sourceStep, uint32_t sourceNumSamples)
{
  int32_t   * __restrict destinSample = (int32_t*)destinPtr;
  float * __restrict sourceSample = (float*)sourcePtr;
  uint32_t minSamples = std::min(destinNumSamples, sourceNumSamples);
  destinStep = destinStep >> 2;
  sourceStep = sourceStep >> 2;
  for (uint32_t cntSamples = 0; cntSamples < minSamples; cntSamples++)
  {
#define VERSION 1
#if VERSION == 0
    float tempFloat = *sourceSample * cFactorFloat32toInt32 + 0.5f;
    if (tempFloat >= 2147483648.0f)
    {
      *destinSample = 2147483647;
    }
    else if (tempFloat <= -2147483648.0f)
    {
      *destinSample = -2147483648;
    }
    else
    {
      *destinSample = static_cast<int32_t>(tempFloat);
    }
#elif VERSION == 1
    if (*sourceSample > 1.0f)
    {
      *destinSample = cInt32Max;
    }
    else if (*sourceSample < -1.0f)
    {
      *destinSample = cInt32Min;
    }
    else
    {
      *destinSample = static_cast<int32_t> (*sourceSample * cFactorFloat32toInt32 + 0.5f);
    }
#endif

    destinSample += destinStep;
    sourceSample += sourceStep;
  }
  for (uint32_t cntSamples = minSamples; cntSamples < destinNumSamples; cntSamples++)
  {
    *destinSample = 0;
    destinSample += destinStep;
  }
}


static void copyChannelFloat32toFloat32(uint8_t *destinPtr, uint32_t destinStep, uint32_t destinNumSamples,
                                        uint8_t *sourcePtr, uint32_t sourceStep, uint32_t sourceNumSamples)
{
  float * __restrict destinSample = (float*)destinPtr;
  float * __restrict sourceSample = (float*)sourcePtr;
  uint32_t minSamples = std::min(destinNumSamples, sourceNumSamples);
  destinStep = destinStep >> 2;
  sourceStep = sourceStep >> 2;
  for (uint32_t cntSamples = 0; cntSamples < minSamples; cntSamples++)
  {
    *destinSample = *sourceSample;
    destinSample += destinStep;
    sourceSample += sourceStep;
  }
  for (uint32_t cntSamples = minSamples; cntSamples < destinNumSamples; cntSamples++)
  {
    *destinSample = 0.0f;
    destinSample += destinStep;
  }
}


/**
 * @brief Public fuction to copy between two audio (ring) buffers, which are desribed
 *        by IasAudioArea structs.
 */

void copyAudioAreaBuffers(IasAudioArea const       *destinAreas,
                          IasAudioCommonDataFormat  destinFormat,
                          uint32_t               destinOffset,
                          uint32_t               destinNumChannels,
                          uint32_t               destinChanIdx,
                          uint32_t               destinNumFrames,
                          IasAudioArea const       *sourceAreas,
                          IasAudioCommonDataFormat  sourceFormat,
                          uint32_t               sourceOffset,
                          uint32_t               sourceNumChannels,
                          uint32_t               sourceChanIdx,
                          uint32_t               sourceNumFrames)
{
  IAS_ASSERT(destinAreas != nullptr);
  IAS_ASSERT(sourceAreas != nullptr);

  // Set the non-interleaved flag if source buffer *and* destination buffer are non-interleaved.
  bool isNonInterleaved = ((static_cast<int32_t>(destinAreas[0].step) == 8 * toSize(destinFormat)) &&
                                (static_cast<int32_t>(sourceAreas[0].step) == 8 * toSize(sourceFormat)));

  void (*copyFunction)(uint8_t*, uint32_t, uint32_t, uint8_t*, uint32_t, uint32_t);
  copyFunction = nullptr;

  // Depending on the data formats of the source buffer and of the
  // destination buffer, identify which copy function shall be applied.
  switch (sourceFormat)
  {
    case eIasFormatInt16:
      switch (destinFormat)
      {
        case eIasFormatInt16:
          copyFunction = isNonInterleaved ? copyNonInterleavedChannel16bit : copyChannelInt16toInt16;
          break;
        case eIasFormatInt32:
          copyFunction = copyChannelInt16toInt32;
          break;
        case eIasFormatFloat32:
          copyFunction = copyChannelInt16toFloat32;
          break;
        default:
          IAS_ASSERT(0); // format is not supported
          break;
      }
      break;
    case eIasFormatInt32:
      switch (destinFormat)
      {
        case eIasFormatInt16:
          copyFunction = copyChannelInt32toInt16;
          break;
        case eIasFormatInt32:
          copyFunction = isNonInterleaved ? copyNonInterleavedChannel32bit : copyChannelInt32toInt32;
          break;
        case eIasFormatFloat32:
          copyFunction = copyChannelInt32toFloat32;
          break;
        default:
          IAS_ASSERT(0); // format is not supported
          break;
      }
      break;
    case eIasFormatFloat32:
      switch (destinFormat)
      {
        case eIasFormatInt16:
          copyFunction = copyChannelFloat32toInt16;
          break;
        case eIasFormatInt32:
          copyFunction = copyChannelFloat32toInt32;
          break;
        case eIasFormatFloat32:
          copyFunction = isNonInterleaved ? copyNonInterleavedChannel32bit : copyChannelFloat32toFloat32;
          break;
        default:
          IAS_ASSERT(0); // format is not supported
          break;
      }
      break;
    default:
      IAS_ASSERT(0); // format is not supported
      break;
  }

  uint32_t minNumChannels = std::min(destinNumChannels, sourceNumChannels);

  // Verify that we do not copy more channels than provided by the source and destination buffers.
  IAS_ASSERT((minNumChannels + destinChanIdx) <= destinAreas[0].maxIndex + 1);
  IAS_ASSERT((minNumChannels + sourceChanIdx) <= sourceAreas[0].maxIndex + 1);

  for (uint32_t cntChannels = 0; cntChannels < minNumChannels; cntChannels++)
  {
    IAS_ASSERT(sourceAreas[cntChannels+sourceChanIdx].start != nullptr);
    IAS_ASSERT(destinAreas[cntChannels+destinChanIdx].start != nullptr);

    uint32_t  sourceStep   = sourceAreas[cntChannels+sourceChanIdx].step >> 3; // step size expressed in bytes
    uint8_t  *sourcePtr    = ((uint8_t*)sourceAreas[cntChannels+sourceChanIdx].start) + (sourceAreas[cntChannels+sourceChanIdx].first >> 3) + sourceOffset * sourceStep;

    uint32_t  destinStep   = destinAreas[cntChannels+destinChanIdx].step >> 3; // step size expressed in bytes
    uint8_t  *destinPtr    = ((uint8_t*)destinAreas[cntChannels+destinChanIdx].start) + (destinAreas[cntChannels+destinChanIdx].first >> 3) + destinOffset * destinStep;

    copyFunction(destinPtr, destinStep, destinNumFrames, sourcePtr, sourceStep, sourceNumFrames);
  }
}

/**
 * @brief Public fuction to fill zeros into an audio (ring) buffer, which is desribed
 *        by an IasAudioArea struct.
 */
void zeroAudioAreaBuffers(IasAudioArea const       *destinAreas,
                          IasAudioCommonDataFormat  destinFormat,
                          uint32_t               destinOffset,
                          uint32_t               destinNumChannels,
                          uint32_t               destinChanIdx,
                          uint32_t               destinNumFrames)
{
  IAS_ASSERT(destinAreas != nullptr);
  IAS_ASSERT( (destinNumChannels+destinChanIdx) <= destinAreas[0].maxIndex + 1);

  for (uint32_t cntChannels = 0; cntChannels < destinNumChannels; cntChannels++)
  {
    IAS_ASSERT(destinAreas[cntChannels+destinChanIdx].start != nullptr);

    uint32_t  destinStep   = destinAreas[cntChannels+destinChanIdx].step >> 3; // step size expressed in bytes
    uint8_t  *destinPtr    = ((uint8_t*)destinAreas[cntChannels+destinChanIdx].start) + (destinAreas[cntChannels+destinChanIdx].first >> 3) + destinOffset * destinStep;

    switch (destinFormat)
    {
      case eIasFormatInt16:
      {
        int16_t* samplePtr = (int16_t*)destinPtr;
        for (uint32_t cntSamples = 0; cntSamples < destinNumFrames; cntSamples++)
        {
          *samplePtr = 0;
          samplePtr += destinStep>>1;
        }
        break;
      }
      case eIasFormatInt32:
      {
        int32_t* samplePtr = (int32_t*)destinPtr;
        for (uint32_t cntSamples = 0; cntSamples < destinNumFrames; cntSamples++)
        {
          *samplePtr = 0;
          samplePtr += destinStep>>2;
        }
        break;
      }
      case eIasFormatFloat32:
      {
        float* samplePtr = (float*)destinPtr;
        for (uint32_t cntSamples = 0; cntSamples < destinNumFrames; cntSamples++)
        {
          *samplePtr = 0.0f;
          samplePtr += destinStep>>2;
        }
        break;
      }
      default:
      {
        IAS_ASSERT(0); // format is not supported
        break;
      }
    }
  }
}


} //namespace IasAudio
