/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAlsaTypeConversion.hpp
 * @date   Oct 2, 2015
 * @version 0.1
 * @brief  Connector that connects the Alsa callback functions with the
 * provided smartx ring buffers
 */

#ifndef IASALSATYPECONVERSION_HPP_
#define IASALSATYPECONVERSION_HPP_

/*
 * Alsa
 */
#include <alsa/asoundlib.h>

/*
 * Common
 */
#include "audio/common/IasAudioCommonTypes.hpp"

namespace IasAudio {


/**
 * @brief Function to convert between Ias and Alsa. Channel count from Ias to Alsa.
 *
 * @param in Ias Channel count to convert.
 * @return unsigned int Channel Count for Alsa.
 */
__attribute__ ((visibility ("default"))) unsigned int convertChannelCountIasToAlsa(const IasAudioCommonChannelCount& in);

/**
 * @brief Function to convert between Ias and Alsa. Channel count from Alsa to Ias.
 *
 * @param in Alsa channel count value.
 * @return IasAudioCommonChannelCount Ias channel count value
 */
__attribute__ ((visibility ("default"))) IasAudioCommonChannelCount convertChannelCountAlsaToIas(const unsigned int& in);

/**
 * @brief Function to convert between Ias and Alsa. Format(int16, int32, float) from Ias to Alsa.
 *
 * @param in Ias pcm format
 * @return snd_pcm_format_t Alsa pcm format.
 */
__attribute__ ((visibility ("default"))) snd_pcm_format_t convertFormatIasToAlsa(const IasAudioCommonDataFormat& in);

/**
 * @brief Function to convert between Ias and Alsa. Format(int16, int32, float) from Alsa to Ias.
 * Since there are a very few formats in the Ias domain, there will be a lot of invalid returns.
 *
 * @param in Alsa pcm format.
 * @return IasAudio::IasAudioCommonDataFormat Ias pcm format.
 */
__attribute__ ((visibility ("default"))) IasAudioCommonDataFormat convertFormatAlsaToIas(const snd_pcm_format_t& in);


/**
 * @brief Generates an access type out of the Ias data ordering and a enum mmap flag,
 * found in IasAudioCommonTypes.
 *
 * @param in The data layout, example: eIasAudioCommonInterleaved
 * @param isMmap Access type, example: eIasRwAccess
 * @return snd_pcm_access_t Alsa access type.
 */
__attribute__ ((visibility ("default"))) snd_pcm_access_t convertAccessTypeIasToAlsa(const IasAudioCommonDataLayout& in, const IasAudioCommonAccess& isMmap);

/**
 * @brief Generates an access type out of the Ias data ordering, defaulting the access to eIasRwAccess.
 *
 * @param in The data layoutm example: eIasAudioCommonInterleaved
 * @return snd_pcm_access_t Alsa access type.
 */
__attribute__ ((visibility ("default"))) snd_pcm_access_t convertAccessTypeIasToAlsa(const IasAudioCommonDataLayout& in);


/**
 * @brief Fills the alsa area with the base pointer, offset, and step size
 *
 * @param in IasAudioArea that contains the values.
 * @param out Alsa Channel Area, that contains less values.
 * @return void
 */
__attribute__ ((visibility ("default"))) void convertAreaIasToAlsa(const IasAudioArea& in, snd_pcm_channel_area_t* out);


}

#endif /* IASALSATYPECONVERSION_HPP_ */
