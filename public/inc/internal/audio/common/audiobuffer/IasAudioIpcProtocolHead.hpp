/*
  * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file IasAudioIpcProtocolHead.hpp
 * @date Oct 5, 2015
 * @version 0.1
 * @brief Can be used to build own protocol.
 * 
 * Use the IasAudioIpcProtocolHead to start a protocol definition.
 * Before this including head and tail the IAS_AUDIO_IPC_MESSAGE_SIZE 
 * and the IAS_AUDIO_IPC_QUEUE_SIZE must be set.
 * 
 * #include "SomeOwnInclude.hpp"
 *
 * //Define the sizes first
 * #define IAS_AUDIO_IPC_MESSAGE_SIZE 100
 * #define IAS_AUDIO_IPC_QUEUE_SIZE 100
 * 
 * // Include the start header
 * #include "audio/audiobuffer/IasAudioIpcProtocolHead.hpp"
 * 
 * 
 * 
 * // ############################################################
 * //  IPC ID Definition Section
 * //  Define any fixed size structure without heap memory here.
 * //  For Template Arguments a typedef is required
 * // Example: ADD_IPC_ID(Ias::CLASS, 1234)
 * 
 * 
 * 
 * 
 * // END IPC ID Definition Section
 * // #############################################################
 * 
 * 
 *
 * To end the protocol definition, use the tail include.
 *
 * #include "audio/audiobuffer/IasAudioIpcProtocolTail.hpp"
 * 
 * NOTE:
 * o The Macro GET_IPC_ID(classname) is provided by the procotol, and is a compile time variable.
 *  Zero is the unvalid id.
 * o All interface functions can be found in IasAudioIpc.hpp. 
 * 
 */

//Include the used macros and the base wrapper class.
#include "IasAudioIpcProtocolMacro.hpp"

namespace IasAudio { // Get the definitions in the correct namespace
