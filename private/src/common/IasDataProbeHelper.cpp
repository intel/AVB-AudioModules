/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasDataProbeHelper.cpp
 * @date   2016
 * @brief
 */


#include "internal/audio/common/IasDataProbeHelper.hpp"
 // #define IAS_ASSERT()

namespace IasAudio {

namespace IasDataProbeHelper {

IasDataProbe::IasResult processQueueEntry(IasProbingQueueEntry &entry,
                                          IasDataProbePtr* probe,
                                          uint32_t probingBufferSize)
{
  std::atomic<bool> probingActive(false);
  return processQueueEntry(entry, probe, &probingActive, probingBufferSize);
}

IasDataProbe::IasResult processQueueEntry(IasProbingQueueEntry &entry,
                                          IasDataProbePtr* probe,
                                          std::atomic<bool> *probingActive,
                                          uint32_t probingBufferSize)
{
  IAS_ASSERT(probingActive != nullptr);
  IasDataProbe::IasResult probeRes;

  if (entry.action == eIasProbingStart)
  {
    if (*probe == nullptr)
    {
      *probe = std::make_shared<IasDataProbe>();
      IAS_ASSERT(*probe != nullptr);
      if (entry.params.isInject)
      {

        probeRes = (*probe)->startInject(entry.params.name,
                                         entry.params.numChannels,
                                         entry.params.sampleRate,
                                         entry.params.dataFormat,
                                         entry.params.startIndex,
                                         probingBufferSize,
                                         entry.params.duration);
      }
      else
      {
        probeRes = (*probe)->startRecording(entry.params.name,
                                            entry.params.numChannels,
                                            entry.params.sampleRate,
                                            entry.params.dataFormat,
                                            entry.params.startIndex,
                                            probingBufferSize,
                                            entry.params.duration);
      }
      if(probeRes == IasDataProbe::eIasOk)
      {
        probingActive->store(true);
      }
      else
      {
        *probe = nullptr;
        return probeRes;
      }
    }
    else
    {
      return IasDataProbe::eIasAlreadyStarted;
    }
  }
  else
  {
    if(*probe != nullptr)
    {
      (*probe)->stop();
      probingActive->store(false);
      *probe = nullptr;
    }
  }
  return IasDataProbe::eIasOk;
}


}
}


