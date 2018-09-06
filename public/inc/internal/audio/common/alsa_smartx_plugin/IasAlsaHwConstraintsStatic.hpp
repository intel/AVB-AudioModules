/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file IasAlsaHwConstraintsStatic.hpp
 * @date Sep 22, 2015
 * @version 0.1
 * @brief Defines a shared memory structure to exchange audio device constrains between processes.
 *
 */

#ifndef IASALSAHWCONSTRAINTS_HPP_
#define IASALSAHWCONSTRAINTS_HPP_

/*
 * System
 */
#include <unistd.h>

/*
 * Boost
 */
#include <boost/container/static_vector.hpp>

/*
 * Ias
 */


/*
 * Common
 */
#include "audio/common/IasAudioCommonTypes.hpp"

namespace IasAudio {


template<class C, size_t S>
using IasShmStaticVector = boost::container::static_vector<C, S>;

using IasAudioDataFormatVector = IasShmStaticVector<IasAudioCommonDataFormat, 10>;
using IasAudioDataLayoutVector = IasShmStaticVector<IasAudioCommonDataLayout, 4>;
using IasAudioChannelCount = IasShmStaticVector<IasAudioCommonChannelCount, 8>;
using IasAudioSampleRateVector = IasShmStaticVector<uint32_t, 10>;
using IasAudioPeriodSizeVector = IasShmStaticVector<uint32_t, 10>;
using IasAudioPeriodCountVector = IasShmStaticVector<uint32_t, 10>;
using IasAudioBufferSizeVector = IasShmStaticVector<uint32_t, 10>;

struct IasAlsaHwConstraintsStatic
{
  /**
   * The constructor needs an allocator from the specific shared memory.
   * It initializes allother
   *
   * @param void_allocator
   */
  IasAlsaHwConstraintsStatic():
    isValid(false)
  {}

  /**
   * Valid flag, if the structure should be used.
   */
  bool isValid;

  /**
   * Struct contains the data formats as IasAudioCommonDataType
   */
  struct
  {
    IasAudioDataFormatVector list; ///< List of the supported formats.
  } formats;

  /**
   * Struct contains the layout of the data as IasAudioCommonDataLayout, interleaved or non-interleaved.
   */
  struct
  {
    IasAudioDataLayoutVector list; ///< List of the supported layouts.
  } access;

  /**
   * Struct contains the channel max/min count or the list of the supported counts.
   * Either the list is filled with values or the min/max is valid.
   */
  struct
  {
    IasAudioChannelCount list;          ///< List of the supported channel counts.
    IasAudioCommonChannelCount min;     ///< Minimum channel count
    IasAudioCommonChannelCount max;     ///< Maximum channel count
  } channels;

  /**
   * Struct contains the supported sample rates. Either the list is filled with values,
   * or the min/max is valid.
   */
  struct
  {
    IasAudioSampleRateVector list;  ///< List of the supported sample rates.
    uint32_t min;                ///< Minimum sample rate
    uint32_t max;                ///< Maximum sample rate
  } rate;

  /**
   * Struct the supported period sizes. Either the list is filled with values,
   * or the min/max is valid.
   */
  struct
  {
    IasAudioPeriodSizeVector list;  ///< List of the supported period sizes.
    uint32_t min;                ///< Minimum period size
    uint32_t max;                ///< Maximum period size
  } period_size;

  /**
   * Struct the supported period count. Either the list is filled with values,
   * or the min/max is valid.
   */
  struct
  {
    IasAudioPeriodCountVector list; ///< List of the supported period counts.
    uint32_t min;                ///< Minimum period count
    uint32_t max;                ///< Maximum period count
  } period_count;

  /**
   * Struct the supported period count. Either the list is filled with values,
   * or the min/max is valid.
   */
  struct
  {
    IasAudioBufferSizeVector list;  ///< List of the supported buffer sizes.
    uint32_t min;                ///< Minimum buffer size
    uint32_t max;                ///< Maximum buffer size
  } buffer_size;

};

}

#endif /* IASALSAHWCONSTRAINTS_HPP_ */
