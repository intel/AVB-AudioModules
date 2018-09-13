/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAudioRingBufferTypes.cpp
 * @date   2016
 * @brief
 */

#include "internal/audio/common/audiobuffer/IasAudioRingBufferTypes.hpp"

namespace IasAudio{

#define STRING_RETURN_CASE(name) case name: return std::string(#name); break
#define DEFAULT_STRING(name) default: return std::string(name)

__attribute__ ((visibility ("default"))) std::string toString(const IasAudioRingBufferStreamingState&  type)
{
  switch(type)
  {
    STRING_RETURN_CASE(eIasRingBuffStreamingStateUndefined);
    STRING_RETURN_CASE(eIasRingBuffStreamingStateRunning);
    STRING_RETURN_CASE(eIasRingBuffStreamingStateStopWrite);
    STRING_RETURN_CASE(eIasRingBuffStreamingStateStopRead);
    DEFAULT_STRING("Unknown Error");
  }
}

#undef STRING_RETURN_CASE
#undef DEFAULT_STRING
}
