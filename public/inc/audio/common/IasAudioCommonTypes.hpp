/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAudioCommonTypes.hpp
 * @date   2015
 * @brief  Common Types for all audio entities
 */

#ifndef IASAUDIOCOMMONTYPES_HPP_
#define IASAUDIOCOMMONTYPES_HPP_

#include <memory>
#include <stdint.h>
#include <list>
#include <map>
#include <set>
#include <string>
#include <string.h> //for memcmp
#include <vector>
#include <new>
#include <iostream>
#include <csignal>



/**
 * @brief Namespace for all audio related definitions and declarations
 */
namespace IasAudio {

#define EMBED_BREAKPOINT raise(SIGINT);

#ifndef DEBUG
/**
 * dummy define IAS_ASSERT for release code
 */
  #define IAS_ASSERT(x)
#else
  /**
   * Definition of an assert makro
   */
  #define IAS_ASSERT(x) \
  if (! (x)) \
  { \
  std::cout << "ERROR!! Assert " << #x << " failed\n"; \
  std::cout << " on line " << __LINE__  << "\n"; \
  std::cout << " in file " << __FILE__ << "\n";  \
  EMBED_BREAKPOINT; \
  }
#endif

#ifdef __cplusplus
# define EXTERN_C_BEGIN  extern "C" {
# define EXTERN_C_END    }
#else

/**
 * define dummy EXTERN_C_BEGIN makro for c library
 */
# define EXTERN_C_BEGIN
/**
 * define dummy EXTERN_C_END makro for c library
 */
# define EXTERN_C_END
#endif

#define IAS_AUDIO_PUBLIC __attribute__ ((visibility ("default")))


#ifndef ARRAYLEN
#define ARRAYLEN(a) (sizeof(a)/sizeof(a[0]))
#endif

/**
  * @brief Type definition for an area of PCM samples
  */
struct IasAudioArea
{
  /**
   * @brief Constructs the audio area as invalid.
   *
   */
  inline IasAudioArea():
    start(nullptr),
    first(0),
    step(0),
    index(0),
    maxIndex(0)
  {;}

  void*         start;    //!< base address of channel samples
  uint32_t      first;    //!< offset to first sample in bits
  uint32_t      step;     //!< samples distance in bits
  uint32_t      index;    //!< channel index
  uint32_t      maxIndex; //!< index of the last channel
};

/**
 * @brief Function to get a IasAudioArea as string.
 */
std::string toString(const IasAudioArea&  type);

/**
  * @brief Ring buffer access type (read or write access).
  */
enum IasRingBufferAccess
{
  eIasRingBufferAccessUndef = 0,  //!< Undefined
  eIasRingBufferAccessRead,       //!< Read access
  eIasRingBufferAccessWrite       //!< Write access
};

/**
 * @brief Function to get a IasRingBufferAccess as string.
 *
 * @return Enum Member
 * @return eIasRingBufferAccessInvalid on unknown value.
 */
std::string toString(const IasRingBufferAccess&  type);

/**
  * @brief Ring buffer type.
  */
enum IasRingbufferType
{
  eIasRingBufferUndef = 0,        //!< Undefined
  eIasRingBufferShared,           //!< Ring buffer located in shared memory
  eIasRingBufferLocalMirror,      //!< Ring buffer using the mmap'ed areas of an ALSA device
  eIasRingBufferLocalReal         //!< Ring buffer located in local memory
};

/**
 * @brief Function to get a IasRingbufferType as string.
 *
 * @return Enum Member
 * @return eIasRingBufferInvalid on unknown value.
 */
std::string toString(const IasRingbufferType&  type);

/**
 * @brief Port direction
 */
enum IasPortDirection
{
  eIasPortDirectionUndef = 0,   //!< Undefined
  eIasPortDirectionInput,       //!< Input port
  eIasPortDirectionOutput,      //!< Output port
};

/**
 * @brief Function to get a IasPortDirection as string.
 *
 * @return Enum Member
 * @return eIasPortDirectionInvalid on unknown value.
 */
std::string toString(const IasPortDirection&  type);

/**
 * @brief Device type
 */
enum IasDeviceType
{
  eIasDeviceTypeUndef = 0,   //!< Undefined
  eIasDeviceTypeSource,      //!< Playback device
  eIasDeviceTypeSink,        //!< Capture device
};

/**
 * @brief Function to get a IasDeviceDirection as string.
 *
 * @return Enum Member as string
 * @return eIasDeviceTypeInvalid on unknown value.
 */
std::string toString(const IasDeviceType&  type);

/**
 * @brief Clock type to specify the type of a source or sink device.
 */
enum IasClockType
{
  eIasClockUndef = 0,    //!< Undefined
  eIasClockProvided,     //!< The clock is provided to the external application (alsa-smartx-plugin).
  eIasClockReceived,     //!< The clock is received from the external ALSA device (ALSA Handler).
  eIasClockReceivedAsync //!< The ALSA device runs at the received clock, which is asynchronous to the
                         //!< SmartXBar clock (ALSA Handler with asynchronous sample rate converter).
};

/**
 * @brief Function to get a IasClockType as string.
 *
 * @return Enum Member
 * @return eIasClockInvalid on unknown value.
 */
std::string toString(const IasClockType&  type);

/**
 * @brief Common Result type of the audio domain.
 * Result state of functions can be one of these values.
 *
 */
enum IasAudioCommonResult
{
  eIasResultOk = 0,
  eIasResultInitFailed,
  eIasResultInvalidParam,
  eIasResultNotInitialized,
  eIasResultMemoryError,
  eIasResultTimeOut,
  eIasResultCondWaitMutexOwn,
  eIasResultCondWaitParam,
  eIasResultUnknown,
  eIasResultObjectNotFound,
  eIasResultClockDomainRate,
  eIasResultInvalidSampleSize,
  eIasResultObjectAlreadyExists,
  eIasResultInvalidShmPath,
  eIasResultBufferEmpty,
  eIasResultBufferFull,
  eIasResultAlreadyInitialized,
  eIasResultNotAllowed,
  eIasResultAlsaError,
  eIasResultCRCError,
  eIasResultInvalidSegmentSize,
  eIasResultSinkAlreadyConnected,
  eIasResultConnectionAlreadyExists,
  eIasResultFailed
};

__attribute__ ((visibility ("default"))) IasAudioCommonResult __attribute__((warn_unused_result)) ias_safe_memcpy(void * dest, size_t dest_size, const void * source, size_t source_size);


/**
 * @brief Scheduler priority for threads.
 * Controls priority of worker and runner threads.
 */
enum IasRunnerThreadSchedulePriority
{
  eIasPriorityNormal = 0, //!< Default priority, meant for worker threads
  eIasPriorityOneLess,    //!< Priority one point lower than default, meant for runner threads
};

/**
 * @brief Function to get a IasAudioCommonResult as string.
 *
 * @return Enum Member
 * @return eIasResultInvalid on unknown value.
 */
std::string toString(const IasAudioCommonResult&  type);

/**
 * @brief The audio sample data format types
 */
enum IasAudioCommonDataFormat
{
  eIasFormatUndef = 0,      //!< Undefined
  eIasFormatFloat32,        //!< IEEE Floating Point 32-bit (single precision)
  eIasFormatInt16,          //!< Signed 16-bit integer
  eIasFormatInt32           //!< Signed 32-bit integer
};

/**
 * @brief Function to get a IasAudioCommonDataFormat as string.
 *
 * @return Enum Member
 * @return eIasFormatInvalid on unknown value.
 */
std::string toString(const IasAudioCommonDataFormat&  type);

/**
 * @brief Function to get the size of a IasAudioCommonDataFormat.
 *
 * @return Size, expressed in bytes.
 * @return -1 on eIasFormatUndef or eIasFormatInvalid
 */
int32_t toSize(const IasAudioCommonDataFormat& type);

/**
 * @brief Used to specify the ALSA access type of the plugin/handler.
 *
 */
enum IasAudioCommonAccess
{
  eIasAccessUndef = 0,    //!< Undefined
  eIasAccessMmap,         //!< Memory mapped access
  eIasAccessRw            //!< Read/write access
};

/**
 * @brief Function to get a IasAudioCommonAccess as string.
 *
 * @return Enum Member
 * @return eIasAccessInvalid on unknown value.
 */
std::string toString(const IasAudioCommonAccess&  type);

/**
  * @brief Audio sample buffer layout schemes for the common audio components
  */
enum IasAudioCommonDataLayout
{
  eIasLayoutUndef = 0,      //!< Undefined
  eIasLayoutInterleaved,    //!< Interleaved audio samples
  eIasLayoutNonInterleaved  //!< Non-interleaved audio samples
};

/**
 * @brief Function to get a IasAudioCommonDataLayout as string.
 *
 * @return Enum Member
 * @return eIasLayoutInvalid on unknown value.
 */
std::string toString(const IasAudioCommonDataLayout&  type);

/**
 * @brief Parameter for audio device configuration
 */
struct IasAudioDeviceParams
{
  //! The default constructor
  IasAudioDeviceParams()
    :name()
    ,numChannels(0)
    ,samplerate(0)
    ,dataFormat(eIasFormatUndef)
    ,clockType(eIasClockUndef)
    ,periodSize(0)
    ,numPeriods(0)
    ,numPeriodsAsrcBuffer(4) //!< The recommended size of the ASRC buffer: 4 periods of PCM frames.
  {}

  //! Constructor with most device parameters, except numPeriodsAsrcBuffer, which is initialized with its recommended default value (4 periods).
  IasAudioDeviceParams(std::string p_name,
      uint32_t p_numChannels,
      uint32_t p_samplerate,
      IasAudioCommonDataFormat p_dataFormat,
      IasClockType p_clockType,
      uint32_t p_periodSize,
      uint32_t p_numPeriods)
    :name(p_name)
    ,numChannels(p_numChannels)
    ,samplerate(p_samplerate)
    ,dataFormat(p_dataFormat)
    ,clockType(p_clockType)
    ,periodSize(p_periodSize)
    ,numPeriods(p_numPeriods)
    ,numPeriodsAsrcBuffer(4)
  {}

  //! Constructor with all device parameters, including numPeriodsAsrcBuffer, which is required only if the device is serviced by an asynchronous ALSA handler.
  IasAudioDeviceParams(std::string p_name,
      uint32_t p_numChannels,
      uint32_t p_samplerate,
      IasAudioCommonDataFormat p_dataFormat,
      IasClockType p_clockType,
      uint32_t p_periodSize,
      uint32_t p_numPeriods,
      uint32_t p_numPeriodsAsrcBuffer)
    :name(p_name)
    ,numChannels(p_numChannels)
    ,samplerate(p_samplerate)
    ,dataFormat(p_dataFormat)
    ,clockType(p_clockType)
    ,periodSize(p_periodSize)
    ,numPeriods(p_numPeriods)
    ,numPeriodsAsrcBuffer(p_numPeriodsAsrcBuffer)
  {}

  std::string name;                       //!< Name of the audio device. This is the ALSA PCM device name
  uint32_t numChannels;                   //!< Number of channels
  uint32_t samplerate;                    //!< The sample rate in Hz, e.g. 48000
  IasAudioCommonDataFormat dataFormat;    //!< The data format, see IasAudioCommonDataFormat for details
  IasClockType clockType;                 //!< Whether the audio devices provides the clock to the external application or it receives the clock from the external ALSA PCM device
  uint32_t periodSize;                    //!< The period size in frames
  uint32_t numPeriods;                    //!< The number of periods that the ring buffer consists of
  uint32_t numPeriodsAsrcBuffer;          //!< The number of periods of the ASRC buffer (if the device is serviced by an asynchronous ALSA handler).
};

/**
 * @brief Parameter for audio port configuration
 */
struct IasAudioPortParams
{
  IasAudioPortParams()
    :name()
    ,numChannels(0)
    ,id(-1)
    ,direction(eIasPortDirectionUndef)
    ,index(0xFFFFFFFF)
  {}
  IasAudioPortParams(std::string p_name,
      uint32_t p_numChannels,
      int32_t p_id,
      IasPortDirection p_direction,
      uint32_t p_index)
    :name(p_name)
    ,numChannels(p_numChannels)
    ,id(p_id)
    ,direction(p_direction)
    ,index(p_index)
  {}
  std::string name;             //!< The name of the audio port
  uint32_t numChannels;         //!< The number of channels of the audio port
  int32_t id;                   //!< The source or sink id of the audio port which can be used in the IasIRouting::connect and IasIRouting::disconnect calls
  IasPortDirection direction;   //!< The port directon
  uint32_t index;               //!< The zero based channel index of the first channel of this audio port in the audio device
};

/**
 * @brief Parameters for routing zone configuration
 */
struct IasRoutingZoneParams
{
  IasRoutingZoneParams()
    :name()
  {}
  IasRoutingZoneParams(std::string p_name)
    :name(p_name)
  {}
  std::string name;       //!< The name of the audio routing zone
};


/**
 * @brief IasAudioTimestamp represents a pair of timestamp and numTransmittedFrames
 */
struct IasAudioTimestamp
{
  /**
   * @brief Constructs the AudioTimestamp with initial values.
   *
   */
  inline IasAudioTimestamp()
    :timestamp(0)
    ,numTransmittedFrames(0)
  {}

  uint64_t  timestamp;            //!< timestamp with microseconds resolution
  uint64_t  numTransmittedFrames; //!< number of transmitted PCM frames
};


/**
 * @brief Parameters for pipeline configuration
 *
 * @note The period size has to be a multiple of 4 when using the gcc compiler and a multiple of 8 when using the Intel compiler. This
 * is required due to optimizations being applied to the setup of the processing chain.
 */
struct IasPipelineParams
{
    /**
     * @brief Constructs the IasPipelineParams with initial values.
     */
    inline IasPipelineParams()
      :name()
      ,samplerate(0)
      ,periodSize(0)
    {}

    /**
     * @brief Constructor for initializer list
     *
     * @param[in] p_name Name of the pin
     */
    inline IasPipelineParams(std::string p_name, uint32_t p_samplerate, uint32_t p_periodSize)
      :name(p_name)
      ,samplerate(p_samplerate)
      ,periodSize(p_periodSize)
    {}

    std::string name;           //!< The name of the pipeline
    uint32_t samplerate;        //!< The sample rate in Hz, e.g. 48000
    uint32_t periodSize;        //!< The period size in frames. It has to be a multiple of 4 when using the gcc compiler and a multiple of 8 when using the Intel compiler.
};


/**
 * @brief Parameters for audio pin configuration
 */
struct IasAudioPinParams
{
    /**
     * @brief Constructs the IasAudioPinParams with initial values.
     */
    inline IasAudioPinParams()
      :name()
      ,numChannels(0)
    {}

    /**
     * @brief Constructor for initializer list
     *
     * @param[in] p_name Name of the pin
     * @param[in] p_numChannels Number of channels of the pin
     */
    inline IasAudioPinParams(std::string p_name, uint32_t p_numChannels)
      :name(p_name)
      ,numChannels(p_numChannels)
    {}

    std::string name;           //!< The name of the pin
    uint32_t numChannels;    //!< The number of channels of the pin
};


/**
  * @brief Link type between audio output pin and audio input pin.
  */
enum IasAudioPinLinkType
{
  eIasAudioPinLinkTypeImmediate = 0, //!< Immediate (non-delayed) link between output pin and input pin.
  eIasAudioPinLinkTypeDelayed,       //!< Delayed link between output pin and input pin.
};

/**
 * @brief Function to get a IasAudioPinLinkType as string.
 *
 * @return Enum Member
 */
std::string toString(const IasAudioPinLinkType&  type);


/**
 * @brief Parameters for processing module configuration
 */
struct IasProcessingModuleParams
{
    /**
     * @brief Constructs the IasProcessingModuleParams with initial values.
     */
    inline IasProcessingModuleParams()
      :typeName()
      ,instanceName()
    {}

    /**
     * @brief Constructor for initializer list
     *
     * @param[in] p_typeName Module type name
     * @param[in] p_instanceName Module instance name
     */
    inline IasProcessingModuleParams(std::string p_typeName, std::string p_instanceName)
      :typeName(p_typeName)
      ,instanceName(p_instanceName)
    {}

    std::string typeName;       //!< Module type name
    std::string instanceName;   //!< Module instance name
};


/**
 * @brief Ias Audio Channel Count, 0 is handled as invalid.
 */
typedef uint32_t IasAudioCommonChannelCount;

class IasRoutingZone;
/**
 * @brief Shared ptr type for IasRoutingZone
 */
using IasRoutingZonePtr = std::shared_ptr<IasRoutingZone>;

class IasAudioDevice;
/**
 * @brief Shared ptr type for IasAusioDevice
 */
using IasAudioDevicePtr = std::shared_ptr<IasAudioDevice>;

class IasAudioSourceDevice;
/**
 * @brief Shared ptr type for IasAusioSourceDevice
 */
using IasAudioSourceDevicePtr = std::shared_ptr<IasAudioSourceDevice>;

class IasAudioSinkDevice;
/**
 * @brief Shared ptr type for IasAudioSinkDevice
 */
using IasAudioSinkDevicePtr = std::shared_ptr<IasAudioSinkDevice>;

class IasAudioPort;
/**
 * @brief Shared ptr type for IasAudioPort
 */
using IasAudioPortPtr = std::shared_ptr<IasAudioPort>;

/**
 * @brief Shared ptr type for IasAudioDeviceParams
 */
using IasAudioDeviceParamsPtr = std::shared_ptr<IasAudioDeviceParams>;

/**
 * @brief Shared ptr type for IasAudioPortParams
 */
using IasAudioPortParamsPtr = std::shared_ptr<IasAudioPortParams>;

/**
 * @brief Shared ptr type for IasRoutingZoneParams
 */
using IasRoutingZoneParamsPtr = std::shared_ptr<IasRoutingZoneParams>;

class IasEvent;
/**
 * @brief Shared ptr type for IasEvent
 */
using IasEventPtr = std::shared_ptr<IasEvent>;

class IasConnectionEvent;
/**
 * @brief Shared ptr type for IasConnectionEvent
 */
using IasConnectionEventPtr = std::shared_ptr<IasConnectionEvent>;

class IasSetupEvent;
/**
 * @brief Shared ptr type for IasSetupEvent
 */
using IasSetupEventPtr = std::shared_ptr<IasSetupEvent>;

class IasModuleEvent;
/**
 * @brief Shared ptr type for IasSetupEvent
 */
using IasModuleEventPtr = std::shared_ptr<IasModuleEvent>;

/**
 * @brief Type for a pair audio port shared pointers, source port is first, sink port is second
 */
using IasConnectionPortPair = std::pair<IasAudioPortPtr,IasAudioPortPtr>;

/**
 * @brief Vector of connection pairs
 */
using IasConnectionVector = std::vector<IasConnectionPortPair>;

/**
 * @brief Map containing the logical source groups
 */
using IasSourceGroupMap = std::map<std::string,std::set<int32_t>>;

/**
 * @brief Set containing the IDs of the sources, for which the data must be taken out of the ALSA device, but has to be ignored
 */
using IasDummySourcesSet = std::set<int32_t>;

class IasPipeline;
/**
 * @brief Shared ptr type for IasPipeline
 */
using IasPipelinePtr = std::shared_ptr<IasPipeline>;

/**
 * @brief Shared ptr type for IasPipelineParams
 */
using IasPipelineParamsPtr = std::shared_ptr<IasPipelineParams>;

class IasProcessingModule;
/**
 * @brief Shared ptr type for IasProcessingModule
 */
using IasProcessingModulePtr = std::shared_ptr<IasProcessingModule>;

/**
 * @brief Shared ptr type for IasProcessingModuleConfig
 */
using IasProcessingModuleParamsPtr = std::shared_ptr<IasProcessingModuleParams>;

class IasAudioPin;
/**
 * @brief Shared ptr type for IasAudioPin
 */
using IasAudioPinPtr = std::shared_ptr<IasAudioPin>;

/**
 * @brief Shared ptr type for IasAudioPinParams
 */
using IasAudioPinParamsPtr = std::shared_ptr<IasAudioPinParams>;

/**
 * @brief int64_t vector
 */
using IasInt64Vector = std::vector<int64_t>;
/**
 * @brief int32_t vector
 */
using IasInt32Vector = std::vector<int32_t>;
/**
 * @brief double vector
 */
using IasFloat64Vector = std::vector<double>;
/**
 * @brief float vector
 */
using IasFloat32Vector = std::vector<float>;
/**
 * @brief std::string vector
 */
using IasStringVector = std::vector<std::string>;

class IasProperties;
/**
 * @brief Shared ptr type for IasProperties
 */
using IasPropertiesPtr = std::shared_ptr<IasProperties>;

class IasConfiguration;
/**
 * @brief Shared ptr type for IasProperties
 */
using IasConfigurationPtr = std::shared_ptr<IasConfiguration>;

class IasDataProbe;

/**
 * @brief Shared ptr type for IasDataProbe
 */
using IasDataProbePtr = std::shared_ptr<IasDataProbe>;

} // Namespace IasAudio


#endif // IASAUDIOCOMMONTYPES_HPP_
