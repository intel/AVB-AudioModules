/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAudioRingBufferResult.cpp
 * @date   2015
 * @brief
 */

#include "internal/audio/common/audiobuffer/IasAudioRingBufferResult.hpp"

namespace IasAudio{

#define STRING_RETURN_CASE(name) case name: return std::string(#name); break
#define DEFAULT_STRING(name) default: return std::string(name)

__attribute__ ((visibility ("default"))) std::string toString(const IasAudioRingBufferResult&  type)
{
  switch(type)
  {
    STRING_RETURN_CASE(eIasRingBuffOk);
    STRING_RETURN_CASE(eIasRingBuffInvalidParam);
    STRING_RETURN_CASE(eIasRingBuffNotInitialized);
    STRING_RETURN_CASE(eIasRingBuffAlsaError);
    STRING_RETURN_CASE(eIasRingBuffAlsaXrunError);
    STRING_RETURN_CASE(eIasRingBuffAlsaSuspendError);
    STRING_RETURN_CASE(eIasRingBuffNotAllowed);
    STRING_RETURN_CASE(eIasRingBuffInvalidSampleSize);
    STRING_RETURN_CASE(eIasRingBuffTimeOut);
    STRING_RETURN_CASE(eIasRingBuffProbeError);
    STRING_RETURN_CASE(eIasRingBuffCondWaitFailed);
    DEFAULT_STRING("Unknown Error");
  }
}

#undef STRING_RETURN_CASE
#undef DEFAULT_STRING
}
