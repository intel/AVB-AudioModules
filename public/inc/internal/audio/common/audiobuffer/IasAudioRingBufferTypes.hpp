/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAudioRingBufferTypes.hpp
 * @date   2016
 * @brief  Type definitions for IasAudioRingBuffer, IasAudioRingBufferReal, and IasAudioRingBufferMirror objects.
 */

#ifndef IASAUDIORINGBUFFERTYPES_HPP_
#define IASAUDIORINGBUFFERTYPES_HPP_

#include "audio/common/IasAudioCommonTypes.hpp"

namespace IasAudio {

/*
 * Streaming state of a ring buffer (only supported by real buffers).
 */
enum IasAudioRingBufferStreamingState
{
  eIasRingBuffStreamingStateUndefined = 0, //!< not defined, e.g., in case of a mirror buffer
  eIasRingBuffStreamingStateRunning,       //!< it is possible to write into and to write from the buffer
  eIasRingBuffStreamingStateStopWrite,     //!< write accesses are blocked (PCM frames are discarded)
  eIasRingBuffStreamingStateStopRead       //!< read accesses are blocked (no PCM frames are provided)
};

std::string toString(const IasAudioRingBufferStreamingState&  type);

}
#endif
