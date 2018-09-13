/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file IasAlsaPluginIpc.hpp
 * @date Oct 5, 2015
 * @version 0.1
 * @brief Defines the IPC Protocol between the alsa smartx plugin and the smartx
 */
#ifndef IAS_ASLAPLUGINIPC_HPP_
#define IAS_ASLAPLUGINIPC_HPP_

#include "IasSmartXPluginIpcStructures.hpp"

#define IAS_AUDIO_IPC_MESSAGE_SIZE 100
#define IAS_AUDIO_IPC_QUEUE_SIZE 100

#include "internal/audio/common/audiobuffer/IasAudioIpcProtocolHead.hpp"
// ############################################################
//  IPC ID Definition Section
//  Define any fixed size structure without heap memory here.
//  For Template Arguments a typedef is required
// Example: ADD_IPC_ID(Ias::CLASS, 1234)


/*
 * IPC Traffic between the Alsa SmartX Plugin and the SmartX .
 */
ADD_IPC_ID(IasAudioIpcPluginControl, 1);

/*
 * Contains one floating point with one control type information.
 */
using IasAudioIpcPluginFloatData = IasAudio::IasAudioIpcPluginControlData<float>;
ADD_IPC_ID(IasAudioIpcPluginFloatData, 2);


/*
 * Dummy function to avoid compiler warning "unused function"
 */
inline void dummyCallControl(){(void)GET_IPC_ID(IasAudioIpcPluginControl);};


/*
 * Dummy function to avoid compiler warning "unused function"
 */
inline void dummyCallFloat(){(void)GET_IPC_ID(IasAudioIpcPluginFloatData);};

/*
 * Contains one 32bit integer with one control type information.
 */
using IasAudioIpcPluginInt32Data = IasAudio::IasAudioIpcPluginControlData<int32_t>;
ADD_IPC_ID(IasAudioIpcPluginInt32Data, 3);

/*
 * Dummy function to avoid compiler warning "unused function"
 */
inline void dummyCallInt32(){(void)GET_IPC_ID(IasAudioIpcPluginInt32Data);};

/*
 * Contains the parameter data with one control type information.
 */
using IasAudioIpcPluginParamData = IasAudio::IasAudioIpcPluginControlData<IasAudio::IasAudioCurrentSetParameters>;
ADD_IPC_ID(IasAudioIpcPluginParamData, 4);

/*
 * Dummy function to avoid compiler warning "unused function"
 */
inline void dummyCallParam(){(void)GET_IPC_ID(IasAudioIpcPluginParamData);};

/*
 * Contains the response of a control, which is also represented by a control.
 */
using IasAudioIpcPluginControlResponse = IasAudio::IasAudioIpcPluginControlData<IasAudio::IasAudioIpcPluginControl>;
ADD_IPC_ID(IasAudioIpcPluginControlResponse, 5);

/*
 * Dummy function to avoid compiler warning "unused function"
 */
inline void dummyCallResponse(){(void)GET_IPC_ID(IasAudioIpcPluginControlResponse);};

// END IPC ID Definition Section
// #############################################################

#include "internal/audio/common/audiobuffer/IasAudioIpcProtocolTail.hpp"

#endif /* IAS_ASLAPLUGINIPC_HPP_ */
