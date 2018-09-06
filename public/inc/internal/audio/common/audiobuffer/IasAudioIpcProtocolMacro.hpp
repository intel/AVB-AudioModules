/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file IasAudioIpcProtocolMacro.hpp
 * @date Sep 23, 2015
 * @version 0.2
 * @brief Defines a protocol use by the IPC communication. The Head Macros and wrapper Strucuture. Also Opens a namespace IasAudio.
 * WARNING: Use the IasAudioIpcProtocolHead.hpp and IasAudioIpcProtocolTail.hpp
 */
#ifndef IASAUDIOIPCPROTOCOL_HPP_
#define IASAUDIOIPCPROTOCOL_HPP_

/*
 * System Includes
 */
#include <array>

/*
 * BOOST Includes
 */
#include <boost/crc.hpp>

/*
 * Ias Includes
 */


/*
 * IPC Includes
 */


#ifndef IAS_AUDIO_IPC_MESSAGE_SIZE
#error The IAS_AUDIO_IPC_MESSAGE_SIZE is not defined.
#endif

namespace IasAudio{

/**
  * IasIpcId type is for internal use.
  */
typedef uint32_t IasIpcId;

/**
  * Will be specialized with the different types. Compile error if not specified.
  */
template<typename C>
static IasIpcId getIpcId();

/**
 * This Macro is used to make a class IPC capable. With the classname and a unique id,
 * The class can be sent over the IPC. Zero is a non valid id. The macro will check if the
 * Message size is bigger than the maximum
 */
#define ADD_IPC_ID(classname, id)\
static_assert(sizeof(classname) <= IAS_AUDIO_IPC_MESSAGE_SIZE,\
"Class #classname is too large for the protocol Message size");\
static_assert(id, "Class ipc id is zero:"#classname);\
template<>\
IasIpcId getIpcId<classname>()\
{\
  return id;\
}

/**
 * Macro returns the type number of a specific class.
 */
#define GET_IPC_ID(classname) \
getIpcId<classname>()

}

#endif /* IASAUDIOIPCPROTOCOL_HPP_ */
