/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasDataProbeHelper.hpp
 * @date   2016
 * @brief
 */

#ifndef IASDATAPROBEHELPER_HPP_
#define IASDATAPROBEHELPER_HPP_


#include "audio/common/IasAudioCommonTypes.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"
#include "internal/audio/common/IasDataProbe.hpp"
#include <atomic>

namespace IasAudio {

/**
 * @brief Data probe helper methods to ease the usage of the data probe
 */

namespace IasDataProbeHelper {

__attribute__ ((visibility ("default"))) IasDataProbe::IasResult processQueueEntry(IasProbingQueueEntry &entry,
                                                         IasDataProbePtr* probe,
                                                         uint32_t probingBufferSize);

__attribute__ ((visibility ("default"))) IasDataProbe::IasResult processQueueEntry(IasProbingQueueEntry &entry,
                                                         IasDataProbePtr* probe,
                                                         std::atomic<bool> *probingActive,
                                                         uint32_t probingBufferSize);


}

}
#endif
