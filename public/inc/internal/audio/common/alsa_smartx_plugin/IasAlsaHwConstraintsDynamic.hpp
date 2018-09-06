/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file IasAlsaHwConstraintsDynamic.hpp
 * @date Sep 22, 2015
 * @version 0.1
 * @brief Defines a shared memory structure to exchange audio device constrains between processes.
 *
 */

#ifndef IASALSAHWCONSTRAINTS_HPP_
#define IASALSAHWCONSTRAINTS_HPP_

#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>


#include "audio/common/IasAudioCommonTypes.hpp"

namespace IasAudio {

/*
 * Define some type names for aesthetical reasons.
 * It is possible to use 'IasShmDynamicVector<Type-Id> vectorName' as shared memory capable vector.
 *
 * Don't forget the constructor needs a const IasShmAllocator<void>&.
 */
template<class C>
using IasShmAllocator = boost::interprocess::allocator<C, boost::interprocess::managed_shared_memory::segment_manager>;


template<class C>
using IasShmDynamicVector = boost::interprocess::vector<C, IasShmAllocator<C> >;


struct IasAlsaHwConstraintsDynamic
{
  /**
   * The constructor needs an allocator from the specific shared memory.
   * It initializes allother
   *
   * @param void_allocator
   */
  IasAlsaHwConstraintsDynamic(const IasShmAllocator<void>& void_allocator):
    isValid(false),
    formats({IasShmDynamicVector<IasAudioCommonDataType>(void_allocator)}),
    access({IasShmDynamicVector<IasAudioCommonDataLayout>(void_allocator)}),
    channels({IasShmDynamicVector<uint16_t>(void_allocator), 0, 0}),
    rate({IasShmDynamicVector<IasAudioCommonSampleRates>(void_allocator), IasAudioCommonSampleRates(eIas_8000Hz),
          IasAudioCommonSampleRates(eIas_8000Hz)}),
    period_size({IasShmDynamicVector<uint32_t>(void_allocator), 0, 0}),
    period_count({IasShmDynamicVector<uint32_t>(void_allocator), 0, 0}),
    buffer_size({IasShmDynamicVector<uint32_t>(void_allocator), 0, 0})
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
    IasShmDynamicVector<IasAudioCommonDataFormat> list; ///< List of the supported formats.
  } formats;

  /**
   * Struct contains the layout of the data as IasAudioCommonDataLayout, interleaved or non-interleaved.
   */
  struct
  {
    IasShmDynamicVector<IasAudioCommonDataLayout> list; ///< List of the supported layouts.
  } access;

  /**
   * Struct contains the channel max/min count or the list of the supported counts.
   * Either the list is filled with values or the min/max is valid.
   */
  struct
  {
    IasShmDynamicVector<IasAudioCommonChannelCount> list; ///< List of the supported channel counts.
    IasAudioCommonChannelCount min;                ///< Minimum channel count
    IasAudioCommonChannelCount max;                ///< Maximum channel count
  } channels;

  /**
   * Struct contains the supported sample rates. Either the list is filled with values,
   * or the min/max is valid.
   */
  struct
  {
    IasShmDynamicVector<uint32_t> list; ///< List of the supported sample rates.
    uint32_t min;                ///< Minimum sample rate
    uint32_t max;                ///< Maximum sample rate
  } rate;

  /**
   * Struct the supported period sizes. Either the list is filled with values,
   * or the min/max is valid.
   */
  struct
  {
    IasShmDynamicVector<uint32_t> list; ///< List of the supported period sizes.
    uint32_t min;                ///< Minimum period size
    uint32_t max;                ///< Maximum period size
  } period_size;

  /**
   * Struct the supported period count. Either the list is filled with values,
   * or the min/max is valid.
   */
  struct
  {
    IasShmDynamicVector<uint32_t> list; ///< List of the supported period counts.
    uint32_t min;                ///< Minimum period count
    uint32_t max;                ///< Maximum period count
  } period_count;

  /**
   * Struct the supported period count. Either the list is filled with values,
   * or the min/max is valid.
   */
  struct
  {
    IasShmDynamicVector<uint32_t> list; ///< List of the supported buffer sizes.
    uint32_t min;                ///< Minimum buffer size
    uint32_t max;                ///< Maximum buffer size
  } buffer_size;

};

}

#endif /* IASALSAHWCONSTRAINTS_HPP_ */
