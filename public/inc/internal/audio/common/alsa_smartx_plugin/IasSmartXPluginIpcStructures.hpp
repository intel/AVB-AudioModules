/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file IasSmartXPluginIpcStructures.hpp
 * @date Oct 5, 2015
 * @version 0.1
 * @brief Defines the structures that are transfered between the smartx and the smartx alsa plugin via ipc.
 */
#ifndef IAS_SMARTXPLUGIN_IPCSTRUCTURES_HPP_
#define IAS_SMARTXPLUGIN_IPCSTRUCTURES_HPP_


#include "audio/common/IasAudioCommonTypes.hpp"

namespace IasAudio {

/**
  * @brief This Structure defines possible IPC communication commands.
  */
enum IasAudioIpcPluginControl
{
  eIasAudioIpcInvalid = -1,
  eIasAudioIpcNAK,
  eIasAudioIpcACK,
  eIasAudioIpcGetLatency,
  eIasAudioIpcStart,
  eIasAudioIpcPause,
  eIasAudioIpcResume,
  eIasAudioIpcStop,
  eIasAudioIpcDrain,
  eIasAudioIpcParameters
};

/**
 * @brief toString method for IasAudioIpcPluginControl
 *
 * @param[in] type Enum value of IasAudioIpcPluginControl
 *
 * @return Type as string
 */
std::string toString(const IasAudioIpcPluginControl &type);

/**
 * @brief Structure to tell the smartx which parameters has been chosen.
 *
 */
struct IasAudioCurrentSetParameters
{
  uint32_t numChannels;
  uint32_t sampleRate;
  uint32_t periodSize;
  uint32_t numPeriods;
  IasAudioCommonDataFormat dataFormat;
};



/**
 * @brief A Common container for response messages. Will contain the actual command plus the response package.
 * for further types see in the IPC definition.
 *
 */
template<typename ContainedType>
struct IasAudioIpcPluginControlData
{
  IasAudioIpcPluginControlData():
    control(eIasAudioIpcInvalid)
  {;}

  IasAudioIpcPluginControlData(IasAudioIpcPluginControl type, ContainedType theResponse):
    control(type),
    response(theResponse)
  {;}

  IasAudioIpcPluginControl control;
  ContainedType response;
};

}

#endif /* IAS_SMARTXPLUGIN_IPCSTRUCTURES_HPP_ */
