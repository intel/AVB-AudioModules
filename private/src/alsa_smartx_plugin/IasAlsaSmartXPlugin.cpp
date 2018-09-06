/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAlsaSmartXPlugin.cpp
 * @date   20 Sep. 2015
 * @brief  Interface that is loaded by the alsa library dl-loader.
 * Communicates with the a common shared memory audio ring buffer.
 */

#include <stdlib.h>
#include <string.h>
#include <new>
#include <alsa/asoundlib.h>
#include <alsa/pcm_external.h>

#include "audio/common/IasAudioCommonTypes.hpp"
#include "alsa_smartx_plugin/IasAlsaSmartXConnector.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"

/**
 * Plugin Export Function
 */
#ifdef __cplusplus
extern "C" {
#endif
  __attribute__ ((visibility ("default"))) SND_PCM_PLUGIN_DEFINE_FUNC(smartx)
  {
    // Not used
    (void)root;
    int err = 0;

    char *dbgLvl = getenv("DBG_LVL");
    if (dbgLvl != nullptr && strlen(dbgLvl) <=1)
    {
      IasAudio::IasAudioLogging::addDltContextItem("SXP", (DltLogLevelType)std::stoul(dbgLvl), DLT_TRACE_STATUS_ON);
      IasAudio::IasAudioLogging::addDltContextItem("SHM", (DltLogLevelType)std::stoul(dbgLvl), DLT_TRACE_STATUS_ON);
    }

    IasAudio::IasAlsaSmartXConnector* smartXConnector;
    smartXConnector = new IasAudio::IasAlsaSmartXConnector();
    IAS_ASSERT(smartXConnector != nullptr);
    if(smartXConnector->loadConfig(conf) == -EINVAL)
    {
      delete smartXConnector;
      return -EINVAL;
    }
    if((err = smartXConnector->init(name, stream, mode)))
    {
      snd_pcm_t *pcm = smartXConnector->getPCMHandle();
      if (pcm != nullptr)
      {
        // In case the ioplug was already created we have a pcm handle.
        // Closing this handle will automatically free the memory of our smartXConnector
        snd_pcm_close(pcm);
      }
      else
      {
        // In case we got an error before creating the pcm handle we have to free the memory
        // here explicitly.
        delete smartXConnector;
      }
      return err;
    }

    IAS_ASSERT(smartXConnector->getPCMHandle() != nullptr);
    *pcmp = smartXConnector->getPCMHandle();

    char *dbgId = getenv("DBG_ID");
    if (dbgId != nullptr && strlen(dbgId) <=4)
    {
      DLT_REGISTER_APP(dbgId, "SmartX Plugin");
      DLT_VERBOSE_MODE();
    }
    return 0;

  }

  __attribute__ ((visibility ("default"))) SND_PCM_PLUGIN_SYMBOL(smartx)

#ifdef __cplusplus
}
#endif
