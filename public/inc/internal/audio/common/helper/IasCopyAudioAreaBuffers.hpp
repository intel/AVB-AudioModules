/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasCopyAudioAreaBuffers.hpp
 * @date   2015
 * @brief  Fuction to copy between two audio (ring) buffers, which are desribed
 *         by IasAudioArea structs.
 */

#ifndef IASCOPYAUDIOAREABUFFERS_HPP
#define IASCOPYAUDIOAREABUFFERS_HPP


#include "audio/common/IasAudioCommonTypes.hpp"


namespace IasAudio {

/**
 * @brief Fuction to copy between two audio (ring) buffers, which are desribed
 *        by IasAudioArea structs.
 *
 * If the destination buffer expects more PCM frames than provided by the source buffer,
 * the remaining PCM frames are filled with sine waves (with non-contiguous phase).
 * This has to be changed later (i.e. the sine waves have to be replaced by zeros).
 *
 * @param[in]  destinAreas        Vector of audio areas that describe the buffer layout of the destination buffer.
 * @param[in]  destinFormat       Data format of the destination buffer.
 * @param[in]  destinOffset       Sample offset for the destination buffer.
 * @param[in]  destinNumChannels  Number of channels to be written to the destination buffer.
 * @param[in]  destinChanIdx      Starting channel index of destination area, where to put data
 * @param[in]  destinNumFrames    Number of frames to be written to the destination buffer.
 * @param[in]  sourceAreas        Vector of audio areas that describe the buffer layout of the source buffer.
 * @param[in]  sourceFormat       Data format of the source buffer.
 * @param[in]  sourceOffset       Sample offset for the source buffer.
 * @param[in]  sourceNumChannels  Number of channels to be read from the source buffer.
 * @param[in]  sourceChanIdx      Starting channel index of source area, where to get data from
 * @param[in]  sourceNumFrames    Number of frames to be read from the source buffer.
 */

__attribute__ ((visibility ("default"))) void copyAudioAreaBuffers(IasAudioArea const       *destinAreas,
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
                                         uint32_t               sourceNumFrames);


/**
 * @brief Fuction to fill zeros into an audio (ring) buffer, which is desribed
 *        by an IasAudioArea struct.
 *
 * @param[in]  destinAreas        Vector of audio areas that describe the buffer layout of the destination buffer.
 * @param[in]  destinFormat       Data format of the destination buffer.
 * @param[in]  destinOffset       Sample offset for the destination buffer.
 * @param[in]  destinNumChannels  Number of channels to be written to the destination buffer.
 * @param[in]  destinChanIdx      Start channel index of destination
 * @param[in]  destinNumFrames    Number of frames to be written to the destination buffer.
 */
__attribute__ ((visibility ("default"))) void zeroAudioAreaBuffers(IasAudioArea const       *destinAreas,
                                         IasAudioCommonDataFormat  destinFormat,
                                         uint32_t               destinOffset,
                                         uint32_t               destinNumChannels,
                                         uint32_t               destinChanIdx,
                                         uint32_t               destinNumFrames);


} //namespace IasAudio

#endif // IASCOPYAUDIOAREABUFFERS_HPP
