/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAlsa.cpp
 * @date   2013
 * @brief  This is the wrapper implementation to use the sample rate converter of the SmartXBar
 *         as an ALSA rate converter plugin.
 *
 * To enable this plugin, the asound.conf must include the line
 * defaults.pcm.rate_converter "smartx"
 */

#include <cmath>
#include <iostream>
#include <stdio.h>
#include <alsa/asoundlib.h>
#include <alsa/pcm_rate.h>
#include "audio/common/IasAudioCommonTypes.hpp"
#include "internal/audio/common/samplerateconverter/IasSrcFarrow.hpp"

using namespace IasAudio;
/*
 *  Set PROFILE to 1 to enable time-stamp measurements
 */
#define PROFILE 0


/*
 *  Set VERBOSE to 0, 1, 2, or 3 to select verbosity level.
 *  VERBOSE==0 : print out only errors and warnings (recommended)
 *  VERBOSE==1 : print out information during initialization and during clean-up
 *  VERBOSE==2 : print out information during run-time
 *  VERBOSE==3 : print out even more information during run-time
 */

#define VERBOSE 0

#if PROFILE
#include "internal/audio/smartx_test_support/IasTimeStampCounter.hpp"
#endif


EXTERN_C_BEGIN

struct rate_src {
  IasAudio::IasSrcFarrow *src;
  uint32_t      in_rate;
  snd_pcm_format_t in_format;
  uint32_t      out_rate;
  snd_pcm_format_t out_format;
  float   ratio;
  uint32_t    channels;
  int32_t   **outputBuffers32;
  int32_t   **inputBuffers32;
  int16_t   **outputBuffers16;
  int16_t   **inputBuffers16;
};

static snd_pcm_uframes_t input_frames(void *obj, snd_pcm_uframes_t frames)
{
  snd_pcm_uframes_t in_frames = 0;
  struct rate_src *rate = reinterpret_cast<struct rate_src*>(obj);
  if (rate != NULL)
  {
    in_frames = static_cast<snd_pcm_uframes_t>(static_cast<float>(frames) * rate->ratio);
  }
#if (VERBOSE >= 3)
  fprintf(stderr, "input_frames: obj=%p, out_frames=%u, in_frames=%u\n", obj, (unsigned int)frames, (unsigned int)in_frames);
#endif
  return in_frames;
}

static snd_pcm_uframes_t output_frames(void *obj, snd_pcm_uframes_t frames)
{
  snd_pcm_uframes_t out_frames = 0;
  struct rate_src *rate = reinterpret_cast<struct rate_src*>(obj);
  if (rate != NULL)
  {
    out_frames = static_cast<snd_pcm_uframes_t>(static_cast<float>(frames) / rate->ratio);
  }
#if (VERBOSE >= 3)
  fprintf(stderr, "output_frames: obj=%p, in_frames=%u, out_frames=%u\n", obj, (unsigned int)frames, (unsigned int)out_frames);
#endif
  return out_frames;
}

static void pcm_src_free(void *obj)
{
  struct rate_src *rate = reinterpret_cast<struct rate_src*>(obj);

  delete rate->src;
  free(rate->inputBuffers32);
  free(rate->outputBuffers32);

  rate->src           = NULL;
  rate->outputBuffers32 = NULL;
  rate->inputBuffers32  = NULL;
  rate->outputBuffers16 = NULL;
  rate->inputBuffers16  = NULL;

#if (VERBOSE >= 1)
  fprintf(stderr, "pcm_src_free: obj=%p\n", obj);
#endif
}

static int pcm_src_init(void *obj, snd_pcm_rate_info_t *info)
{
  struct rate_src *rate = reinterpret_cast<struct rate_src*>(obj);

  if (rate->src == NULL)
  {
    rate->outputBuffers32 = NULL;
    rate->inputBuffers32  = NULL;
    rate->outputBuffers16 = NULL;
    rate->inputBuffers16  = NULL;
    rate->src           = NULL;

    rate->in_format = info->in.format;
    rate->out_format = info->out.format;
    if (rate->in_format != rate->out_format)
    {
      fprintf(stderr, "WARNING: Input format = %d, Output format = %d\n", rate->in_format, rate->out_format);
    }
    // Allocate two vectors of pointers (one input pointer and one output
    // pointer for each channel).
    rate->outputBuffers32 = (int32_t**)malloc(info->channels * sizeof(int32_t*));
    rate->inputBuffers32  = (int32_t**)malloc(info->channels * sizeof(int32_t*));
    if ((rate->outputBuffers32 == NULL) || (rate->inputBuffers32 == NULL))
    {
      fprintf(stderr, "Could not allocate outputBuffers and inputBuffers\n");
      pcm_src_free(obj);
      return -ENOMEM;
    }
    rate->outputBuffers16 = reinterpret_cast<int16_t**>(rate->outputBuffers32);
    rate->inputBuffers16 = reinterpret_cast<int16_t**>(rate->inputBuffers32);

    rate->src = new IasAudio::IasSrcFarrow();
    if (rate->src == NULL)
    {
      fprintf(stderr, "Could not allocate Src\n");
      return -ENOMEM;
    }
    IasAudio::IasSrcFarrow::IasResult result = rate->src->init(info->channels);
    if (result != 0)
    {
      fprintf(stderr, "Error initializing Src: %s\n", toString(result).c_str());
      pcm_src_free(obj);
      return -EINVAL;
    }
    // Tell the SRC that it shall use a linear output buffer.
    rate->src->setBufferMode(IasAudio::IasSrcFarrow::eIasLinearBufferMode);

    result = rate->src->setConversionRatio(static_cast<uint32_t>(info->in.rate),
                                           static_cast<uint32_t>(info->out.rate));
    if (result != IasAudio::IasSrcFarrow::eIasOk)
    {
      fprintf(stderr, "Error setting conversion ratio: %s\n", toString(result).c_str());
      pcm_src_free(obj);
      return -EINVAL;
    }
    rate->ratio = static_cast<float>(info->in.rate) / static_cast<float>(info->out.rate);
    rate->channels = info->channels;
  }

#if (VERBOSE >= 1)
  fprintf(stderr, "pcm_src_init: obj=%p, channels=%u, in.rate=%u, out.rate=%u, ratio=%f\n",
          obj,
          info->channels,
          info->in.rate,
          info->out.rate,
          rate->ratio);
  fprintf(stderr, "in.format=%u, out.format=%u\n", info->in.format, info->out.format);
#endif

  return 0;
}

static int pcm_src_adjust_pitch(void *obj, snd_pcm_rate_info_t *info)
{
  struct rate_src *rate = reinterpret_cast<struct rate_src*>(obj);
  if (rate->src != NULL)
  {
    IasAudio::IasSrcFarrow::IasResult result = rate->src->setConversionRatio(static_cast<uint32_t>(info->in.rate),
                                                                             static_cast<uint32_t>(info->out.rate));
    if (result != IasAudio::IasSrcFarrow::eIasOk)
    {
      fprintf(stderr, "Error setting conversion ratio: %s\n", toString(result).c_str());
      return -EINVAL;
    }

    rate->ratio = (static_cast<float>(info->in.period_size) /
                   static_cast<float>(info->out.period_size));

    rate->src->detunePitch(static_cast<uint32_t>(info->in.period_size), static_cast<uint32_t>(info->out.period_size));
  }

#if (VERBOSE >= 1)
  fprintf(stderr, "pcm_src_adjust_pitch: obj=%p, channels=%u, in.rate=%u, out.rate=%u, in.period_size=%lu, out.period_size=%lu, ratio=%f\n",
          obj,
          info->channels,
          info->in.rate,
          info->out.rate,
          info->in.period_size,
          info->out.period_size,
          rate->ratio);
#endif
  return 0;
}

static void pcm_src_reset(void *obj)
{
  struct rate_src *rate = reinterpret_cast<struct rate_src*>(obj);
  if (rate->src != NULL)
  {
    rate->src->reset();
  }
#if (VERBOSE >= 1)
  fprintf(stderr, "pcm_src_reset: obj=%p\n", obj);
#endif
}

static void pcm_src_convert(void *obj,
                            const snd_pcm_channel_area_t *dst_areas,
                            snd_pcm_uframes_t dst_offset,
                            unsigned int dst_frames,
                            const snd_pcm_channel_area_t *src_areas,
                            snd_pcm_uframes_t src_offset,
                            unsigned int src_frames)
{
  // Shift-value for conversion from bits to samples: log2(8*sizeof(uint32_t)) = log2(32) = 5.
  // This constant is used to convert a distance expressed in bits into a distance expressed
  // in samples.
  (void)dst_offset;
  (void)src_offset;
  static const uint32_t cLdWordlenUInt32 = 5;
  static const uint32_t cLdWordlenUInt16 = 4;

#if PROFILE
  static uint64_t timeStampLast = 0;
  uint64_t timeStamp1 =  IasAudio::getTimeStamp64()/1200;
  std::cout << std::endl;
  std::cout << "Time Stamp at start of pcm_src_convert() function: " << timeStamp1 << std::endl;
#endif

  struct rate_src *rate = reinterpret_cast<struct rate_src*>(obj);
#if (VERBOSE >= 3)
  fprintf(stderr, "pcm_src_convert:\n");
  fprintf(stderr, "dst_offset: %u dst_frames: %u\n", (unsigned int)dst_offset, (unsigned int)dst_frames);
  fprintf(stderr, "src_offset: %u src_frames: %u\n", (unsigned int)src_offset, (unsigned int)src_frames);
#endif

  if (rate->src != NULL)
  {
#if (VERBOSE >= 3)
    for (uint32_t channel=0; channel<rate->channels; ++channel)
    {
      fprintf(stderr, "%u:\n", channel);
      fprintf(stderr, "dst_areas->addr: %p, dst_areas->first: %u, dst_areas->step: %u\n",
              dst_areas[channel].addr, dst_areas[channel].first, dst_areas[channel].step);
      fprintf(stderr, "src_areas->addr: %p, src_areas->first: %u, src_areas->step: %u\n",
              src_areas[channel].addr, src_areas[channel].first, src_areas[channel].step);
    }
#endif

    uint32_t  numGeneratedSamples;
    uint32_t  numSkippedSamples;
    uint32_t  writeIndex;                    // Not used, since SRC works in linear output buffer mode.
    uint32_t  const cReadIndex = 0;          // Not required, since SRC works in linear output buffer mode.
    float const cRatioAdjustment = 1.0f; // Constant adjustment, because SRC is synchronous.
    int status = 0;
    (void)status;

    if (rate->in_format == SND_PCM_FORMAT_S32_LE)
    {
      uint32_t dstFrameStepSize = dst_areas[0].step >> cLdWordlenUInt32;
      uint32_t srcFrameStepSize = src_areas[0].step >> cLdWordlenUInt32;
      for (uint32_t channel=0; channel < rate->channels; ++channel)
      {
        rate->outputBuffers32[channel] = reinterpret_cast<int32_t*>(dst_areas[channel].addr) + (dst_areas[channel].first >> cLdWordlenUInt32) + dstFrameStepSize*dst_offset;
        rate->inputBuffers32[channel]  = reinterpret_cast<int32_t*>(src_areas[channel].addr) + (src_areas[channel].first >> cLdWordlenUInt32) + srcFrameStepSize*src_offset;
      }

      status = rate->src->processPushMode(rate->outputBuffers32,
                                          const_cast<const int32_t**>(rate->inputBuffers32),
                                          dstFrameStepSize, /* outputStride */
                                          srcFrameStepSize, /* inputStride  */
                                          &numGeneratedSamples,
                                          &numSkippedSamples,
                                          &writeIndex,
                                          cReadIndex,
                                          dst_frames,                            /* lengthOutputBuffers */
                                          src_frames,                            /* numInputSamples     */
                                          rate->channels,                        /* numChannels         */
                                          cRatioAdjustment);
      (void)status;   // void cast to avoid Klocwork issue
    }
    else if (rate->in_format == SND_PCM_FORMAT_S16_LE)
    {
      uint32_t dstFrameStepSize = dst_areas[0].step >> cLdWordlenUInt16;
      uint32_t srcFrameStepSize = src_areas[0].step >> cLdWordlenUInt16;
      for (uint32_t channel=0; channel < rate->channels; ++channel)
      {
        rate->outputBuffers16[channel] = reinterpret_cast<int16_t*>(dst_areas[channel].addr) + (dst_areas[channel].first >> cLdWordlenUInt16) + dstFrameStepSize*dst_offset;
        rate->inputBuffers16[channel]  = reinterpret_cast<int16_t*>(src_areas[channel].addr) + (src_areas[channel].first >> cLdWordlenUInt16) + srcFrameStepSize*src_offset;
      }

      status = rate->src->processPushMode(rate->outputBuffers16,
                                          const_cast<const int16_t**>(rate->inputBuffers16),
                                          dstFrameStepSize, /* outputStride */
                                          srcFrameStepSize, /* inputStride  */
                                          &numGeneratedSamples,
                                          &numSkippedSamples,
                                          &writeIndex,
                                          cReadIndex,
                                          dst_frames,                            /* lengthOutputBuffers */
                                          src_frames,                            /* numInputSamples     */
                                          rate->channels,                        /* numChannels         */
                                          cRatioAdjustment);
      (void)status;   // void cast to avoid Klocwork issue
    }
    else
    {
      fprintf(stderr, "Unsupported format %d\n", rate->in_format);
    }

#if (VERBOSE >= 2)
    fprintf(stderr, "status: %d:, src_frames = %d, dst_frames = %d, num generated samples = %d, num skipped samples = %d\n",
            status, src_frames, dst_frames, numGeneratedSamples, numSkippedSamples);
#endif
  }

#if PROFILE
  uint64_t timeStamp2 =  IasAudio::getTimeStamp64()/1200;
  std::cout << "Time Stamp at end of pcm_src_convert() function:   " << timeStamp2 << std::endl;
  std::cout << "Time needed for conversioon:                       " << timeStamp2-timeStamp1 << std::endl;
  std::cout << "Time since last call of this function:             " << timeStamp1-timeStampLast << std::endl << std:: endl;
  timeStampLast = timeStamp1;
#endif
}

static void pcm_src_close(void *obj)
{
  // pcm_src_free(obj) does not need to be be called here, because
  // this seems to be done by the alsa-lib.

#if (VERBOSE >= 1)
  fprintf(stderr, "pcm_src_close: obj=%p\n", obj);
#endif
  free(obj);
}

#if SND_PCM_RATE_PLUGIN_VERSION >= 0x010002
static int get_supported_rates(void *obj,
                               unsigned int *rate_min,
                               unsigned int *rate_max)
{
  (void)obj;
  *rate_min =  8000u;
  *rate_max = 48000u;

  //fprintf(stderr, "get_supported_rates: obj=%p, rate_min=%u, rate_max=%u\n", obj, *rate_min, *rate_max);
  return 0;
}

static void dump(void *obj, snd_output_t *out)
{
  (void)obj;
  fprintf(stderr, "dump: obj=%p, out=%p\n", obj, out);
  snd_output_printf(out, "Converter: smartx\n");
}
#endif

static snd_pcm_rate_ops_t pcm_src_ops = {
  /*.close = */pcm_src_close,
  /*.init = */pcm_src_init,
  /*.free = */pcm_src_free,
  /*.reset = */pcm_src_reset,
  /*.adjust_pitch = */pcm_src_adjust_pitch,
  /*.convert = */pcm_src_convert,
  /*.convert_s16 = */NULL,
  /*.input_frames = */input_frames,
  /*.output_frames = */output_frames,
#if SND_PCM_RATE_PLUGIN_VERSION >= 0x010002
  /*.version = */SND_PCM_RATE_PLUGIN_VERSION,
  /*.get_supported_rates = */get_supported_rates,
  /*.dump = */dump,
#endif
};

int pcm_src_open(unsigned int version, void **objp, snd_pcm_rate_ops_t *ops)
{
  struct rate_src *rate;

#if (VERBOSE >= 1)
  fprintf(stderr, "pcm_src_open: version=%x, objp=%p, ops=%p\n", version, objp, ops);
#endif
#if SND_PCM_RATE_PLUGIN_VERSION < 0x010002
  if (version != SND_PCM_RATE_PLUGIN_VERSION)
  {
    fprintf(stderr, "Invalid rate plugin version %x\n", version);
    return -EINVAL;
  }
#endif
  rate = static_cast<struct rate_src*>(calloc(1, sizeof(*rate)));
  if (!rate)
  {
    return -ENOMEM;
  }

  *objp = rate;
  rate->src = NULL;
#if SND_PCM_RATE_PLUGIN_VERSION >= 0x010002
  if (version == 0x010001)
  {
    IasAudioCommonResult result = ias_safe_memcpy(ops,
                                                  sizeof(snd_pcm_rate_ops_t),
                                                  &pcm_src_ops,
                                                  sizeof(snd_pcm_rate_old_ops_t));
    IAS_ASSERT(result == eIasResultOk);
    (void)result;
  }
  else
#endif
  {
    *ops = pcm_src_ops;
  }
  return 0;
}

__attribute__ ((visibility ("default")))
int SND_PCM_RATE_PLUGIN_ENTRY(smartx)(unsigned int version,
                                   void **objp,
                                   snd_pcm_rate_ops_t *ops)
{
  fprintf(stderr, "SND_PCM_RATE_PLUGIN_ENTRY(smartx): version=%x, objp=%p, ops=%p\n", version, objp, ops);
  return pcm_src_open(version, objp, ops);
}

EXTERN_C_END
