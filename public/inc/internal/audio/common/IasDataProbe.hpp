/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasDataProbe.hpp
 * @date   2016
 * @brief
 */

#ifndef IASDATAPROBE_HPP_
#define IASDATAPROBE_HPP_

#include "audio/common/IasAudioCommonTypes.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"
#include <sndfile.h>
#include <list>
#include <atomic>
#include <string>

namespace IasAudio {

class IasAudioLogging;
class IasMemoryAllocator;

/**
 * @brief The action types for data probing
 *
 */
enum IasProbingAction
{
  eIasProbingStart, //!< start of probing
  eIasProbingStop   //!< stop of probing
};


/**
 * @brief Special pin probing parameters, only needed for probing at pins
 *
 */
struct IasPinProbingParams
{
  IasPinProbingParams()
    :streamId(-1)
    ,input(false)
    ,output(false)
  {}

  IasPinProbingParams(int32_t streamId,
                      bool input,
                      bool output)
    :streamId(streamId)
    ,input(input)
    ,output(output)
  {}

  int32_t streamId;
  bool input;
  bool output;
};

/**
 * @brief The parameter structure for data probing
 *
 */
struct IasProbingParams
{
  IasProbingParams()
    :name("")
    ,duration(0)
    ,isInject(false)
    ,numChannels(0)
    ,startIndex(0)
    ,sampleRate(0)
    ,dataFormat(eIasFormatUndef)
    ,pinParams()
  {}

  IasProbingParams(std::string name,
                   uint32_t numSeconds,
                   bool inject,
                   uint32_t numChannels,
                   uint32_t startIndex,
                   uint32_t sampleRate,
                   IasAudioCommonDataFormat format,
                   IasPinProbingParams pinParams = {-1,false,false})
    :name(name)
    ,duration(numSeconds)
    ,isInject(inject)
    ,numChannels(numChannels)
    ,startIndex(startIndex)
    ,sampleRate(sampleRate)
    ,dataFormat(format)
    ,pinParams(pinParams)
  {}

  std::string name;                     //!< file name prefix of the wav file
  uint32_t duration;                 //!< the duration of the probing in seconds
  bool isInject;                   //!< bool flag to indicate if probing is inject or record
  uint32_t numChannels;              //!< the numer of channels to be probed
  uint32_t startIndex;               //!< the start index of th first channel
  uint32_t sampleRate;               //!< the sample rate of the data
  IasAudioCommonDataFormat dataFormat;  //!< the data format ( Int16, Int32 or Float32 )
  IasPinProbingParams pinParams;        //!< pin parameters, only used in case of pipeline probing
};

/**
 * @brief Queue entry for probing
 */
struct __attribute__ ((visibility ("default"))) IasProbingQueueEntry
{
  IasProbingAction action;  //!< start or stop the probing
  IasProbingParams params;  //!< the parameters for the probe operation
};

/**
 * @brief Mode for probing
 */
enum IasDataProbeMode{
  eIasDataProbeInject = 0,
  eIasDataProbeRecord,
  eIasDataProbeInit
};

/**
 * @brief Internal strcture for wave file information
 */
struct IasDataProbeWaveFile
{
    inline IasDataProbeWaveFile():
        file(nullptr),
        name("")
    {
      info.channels = 0;
      info.format = 0;
      info.frames = 0;
      info.samplerate = 0;
      info.sections = 0;
      info.seekable = 0;
    }

    SNDFILE* file;    //!< pointer to the file
    SF_INFO info;     //!< file info structure
    std::string name; //!< name of the file
};

class __attribute__ ((visibility ("default"))) IasDataProbe
{
  public:

    enum IasResult
    {
      eIasOk,               //!< Ok, Operation successful
      eIasFailed,           //!< operation failed, log will give information about error
      eIasFinished,         //!< operation finished
      eIasNoOp,             //!< no operation started
      eIasAlreadyStarted    //!< probing started twice
    };

    IasDataProbe();

    virtual ~IasDataProbe();

    /**
     * @brief start injecting data
     *
     * @param[in] fileNamePrefix prefix of file name
     * @param[in] numChannels the number of channels ( and files ) to be injected
     * @param[in] sampleRate the sampleRate of the data, used to verify wav file parameters
     * @param[in] dataFormat data format, used to verify wav file parameters
     * @param[in] startIndex marks the position wher to inject in the buffer
     * @param[in] bufferSize buffer size for probing in terms of numPeriods* periodSize [frames]
     * @param[in] numSeconds the number of seconds to be injected
     *
     * @returns error code
     * @retval eIasFailed operation failed, see DLT log for more information
     * @retval eIasOk     everything went well
     */
    IasResult startInject(const std::string &fileNamePrefix,
                          uint32_t numChannels,
                          uint32_t sampleRate,
                          IasAudioCommonDataFormat dataFormat,
                          uint32_t startIndex,
                          uint32_t bufferSize,
                          uint32_t numSeconds);

    /**
     * @brief start recording of data into a wav file
     *
     * @param[in] fileNamePrefix prefix of file name
     * @param[in] numChannels the number of channels ( and files ) to be recorded
     * @param[in] sampleRate the sampleRate of the data, used to verify wav file parameters
     * @param[in] dataFormat data format, used to verify wav file parameters
     * @param[in] startIndex marks the position where to record from the buffer
     * @param[in] bufferSize buffer size for probing in terms of numPeriods* periodSize [frames]
     * @param[in] numSeconds the number of seconds to be recorded
     *
     * @returns error code
     * @retval eIasFailed operation failed, see DLT log for more information
     * @retval eIasOk     everything went well
     */
    IasResult startRecording(const std::string &fileNamePrefix,
                             uint32_t numChannels,
                             uint32_t sampleRate,
                             IasAudioCommonDataFormat dataFormat,
                             uint32_t startIndex,
                             uint32_t bufferSize,
                             uint32_t numSeconds);

    /**
     * @brief process a running inject operation
     *
     * @param[in] area the area where the data should be injected/recorded
     * @param[in] offset the offset for the area
     * @param[in] numFrames the number of frames to be probed in this call
     *
     * @returns error code
     * @retval eIasFailed   operation failed, see DLT log for more information
     * @retval eIasOk       everything went well
     * @retval eIasFinished inject was finished because the desired block size could not be read
     *                      from file anymore;
     */
    IasResult process(IasAudioArea* area, uint32_t offset, uint32_t numFrames);

    /**
     * stops a running operation
     */
    void stop();

    /**
     * @brief updates the position in the wav file by a given number of frames
     *
     * @param[in] numFrames the number of frames
     */
    void updateFilePosition(int32_t numFrames);
    
    /**
     * @brief return the status of the probe, started or not started
     *
     * @returns bol value to tell if started
     */
    bool isStarted(){return mStarted;};

  private:

    /*!
     *  @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasDataProbe(IasDataProbe const &other);

    /*!
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasDataProbe& operator=(IasDataProbe const &other);

    IasDataProbe::IasResult checkWaveFileInfos(SF_INFO &info,
                                               uint32_t sampleRate,
                                               IasAudioCommonDataFormat dataFormat);

    void reset();

    /**
     * @brief store data for a running inject operation
     *
     * @param[in] area the area where the data should be injected
     * @param[in] offset the offset for the area
     * @param[in] numFrames the number of frames to be probed in this call
     *
     * @returns error code
     * @retval eIasFailed   operation failed, see DLT log for more information
     * @retval eIasOk       everything went well
     * @retval eIasFinished inject was finished because the desired block size could not be read
     *                      from file anymore;
     */
    IasResult injectData(IasAudioArea* area, uint32_t offset, uint32_t numFrames);

    /**
     * @brief store data for a running recording
     *
     * @param[in] area the area where the data should be injected
     * @param[in] offset the offset for the area
     * @param[in] numFrames the number of frames to be probed in this call
     *
     * @returns error code
     * @retval eIasFailed operation failed, see DLT log for more information
     * @retval eIasOk     everything went well
     */
    IasResult recordData(IasAudioArea* area, uint32_t offset, uint32_t numFrames);


    /**
     * @brief Read Int16 data from a file
     *
     * @param[in] numFrames the number of frames to be read
     *
     * @returns the number of read frames
     */
    int32_t read_short(uint32_t numFrames);

    /**
     * @brief Write Int16 data to a file
     *
     * @param[in] numFrames the number of frames to be written
     *
     * @returns the number of written frames
     */
    int32_t write_short(uint32_t numFrames);

    /**
     * @brief Write Int32 data to a file
     *
     * @param[in] numFrames the number of frames to be written
     *
     * @returns the number of written frames
     */
    int32_t write_int32(uint32_t numFrames);

    /**
     * @brief Read Int32 data from a file
     *
     * @param[in] numFrames the number of frames to be read
     *
     * @returns the number of read frames
     */
    int32_t read_int32(uint32_t numFrames);

    /**
     * @brief Write Float32 data to a file
     *
     * @param[in] numFrames the number of frames to be written
     *
     * @returns the number of written frames
     */
    int32_t write_float32(uint32_t numFrames);

    /**
     * @brief Read Float32 data from a file
     *
     * @param[in] numFrames the number of frames to be read
     *
     * @returns the number of read frames
     */
    int32_t read_float32(uint32_t numFrames);


    DltContext* mLog;
    std::atomic<IasDataProbeMode> mMode;           //!< identify the mode that is currently chosen
    std::atomic<bool> mStarted;                    //!< flag to indicate if a recording/injecting is active
    std::string mFileNamePrefix;                   //!< name of the wav file
    std::list<IasDataProbeWaveFile> mWaveFileList; //!< list containg wave files for channels
    IasAudioCommonDataFormat mDataFormat;          //!< data format to be used
    uint32_t mNumChannels;                      //!< number of channels ( and wave files)
    uint32_t mStartIndex;                       //!< start index where to store to / read from IasAudioArea
    uint32_t mBufferSize;                       //!< the size of one read or write block for a file
    void* mIntermediateBuffer;                     //!< local buffer for intermediate data buffering
    IasAudioArea* mArea;                           //!< local IasAudioArea, uses mIntermediateBuffer
    uint32_t mNumFramesToProcess;               //!< the number of frames to be processed during one operation
    IasMemoryAllocator* mem;                       //!< pointer to the memory allocator object
};

/**
 * @brief Function to get a IasDataProbe::IasResult as string.
 *
 * @return String carrying the result message.
 */
__attribute__ ((visibility ("default"))) std::string toString(const IasDataProbe::IasResult& result);

/**
 * @brief Function to get a IasDataProbe::IasProbingAction as string.
 *
 * @return String carrying the result message.
 */
__attribute__ ((visibility ("default"))) std::string toString(const IasProbingAction& action);


}
#endif
