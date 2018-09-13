/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAudioRingBufferReal.hpp
 * @date   2015
 * @brief
 */

#ifndef IASAUDIORINGBUFFERREAL_H_
#define IASAUDIORINGBUFFERREAL_H_

#include <atomic>
#include <boost/interprocess/offset_ptr.hpp>
#include "audio/common/IasAudioCommonTypes.hpp"

#include "internal/audio/common/audiobuffer/IasAudioRingBufferTypes.hpp"
#include "internal/audio/common/audiobuffer/IasAudioRingBufferResult.hpp"
#include "internal/audio/common/IasIntProcCondVar.hpp"
#include "internal/audio/common/IasIntProcMutex.hpp"

namespace IasAudio {

class IasMetaData;
class IasUserMetaDataFactory;
class IasFdSignal;

class __attribute__ ((visibility ("default"))) IasAudioRingBufferReal
{

  public:

   /**
    * @brief constructor
    */
    IasAudioRingBufferReal();

    /**
     * @brief destructor, virtual by default
     */
    ~IasAudioRingBufferReal();

    /**
     * @brief init function, will call the factory function and init everything
     *
     * @return error code
     */
    IasAudioRingBufferResult init(uint32_t periodSize,
                                  uint32_t nPeriods,
                                  uint32_t nChannels,
                                  IasAudioCommonDataFormat dataFormat,
                                  void* dataBuf,
                                  bool shared,
                                  IasMetaData* metaData);

    IasAudioRingBufferResult updateAvailable(IasRingBufferAccess access, uint32_t *samples);

    IasAudioRingBufferResult beginAccess(IasRingBufferAccess access, uint32_t* offset, uint32_t* frames);

    IasAudioRingBufferResult endAccess(IasRingBufferAccess access, uint32_t offset, uint32_t frames);

    void triggerFdSignal();

    IasAudioRingBufferResult getDataFormat(IasAudioCommonDataFormat *dataFormat) const;

    uint32_t getNumChannels(){return mNumChannels;};

    void* getDataBuffer(){return mDataBuf.get();};

    uint32_t getSampleSize(){return mSampleSize;};

    uint32_t getPeriodSize() const;

    uint32_t getNumberPeriods() const;

    IasAudioRingBufferResult waitWrite(uint32_t numPeriods, uint32_t timeout_ms);

    IasAudioRingBufferResult waitRead(uint32_t numPeriods, uint32_t timeout_ms);

    IasAudioRingBufferResult getTimestamp(IasRingBufferAccess access, IasAudioTimestamp *audioTimestamp);

    void setStreamingState(IasAudioRingBufferStreamingState streamingState) { mStreamingState = streamingState; };

    IasAudioRingBufferStreamingState getStreamingState() { return mStreamingState; };

    uint32_t getReadOffset() { return mReadOffset; };
    uint32_t getWriteOffset() { return mWriteOffset; };

    void resetFromWriter();
    void resetFromReader();

    /**
     * @brief Overwrite current content with zeros
     *
     * This method overwrites the current content of the ringbuffer with zeros. It is only applied to real ring buffers
     * and not to mirror ring buffers.
     * Note: It does NOT change the read or writer pointer or the buffer fill level, it only zeros out
     * the content. This can be used in error situations, when a buffer is completely filled and we
     * are not able to insert another period. It would lead to playback of old samples when sometimes later
     * the client tries to read out samples from the buffer.
     */
    void zeroOut();

    /*!
     * @brief Set the avail_min value
     *
     * The avail_min value specifies how many frames must be available, before a server-side
     * call of the endAccess method may trigger the fdSignal. This is to avoid that a
     * client-side call of the snd_pcm_wait function will return, before there are at least
     * avail_min frames available.
     *
     * For SmartXbar sources (playback devices), the number of available frames means
     * the number of free frames within the buffer.
     * For SmartXbar sinks (capture devices), the number of available frames means
     * the number of filled frames within the buffer.
     *
     * @param[in] availMin  The minimum number of frames that must be available before the fdSignal might be triggered.
     */
    void setAvailMin(uint32_t availMin);

    /*!
     * @brief Set the file descriptor signal instance
     *
     * This method is only relevant for real buffers. It sets the fd signal instance of the ALSA plugin connection.
     * The fd signal instance is used to signal changes of the buffer fill level from the SmartXbar to the user application.
     *
     * @param[in] fdSignal The fd signal instance to be used
     * @param[in] deviceType The device type to decide when to trigger a signal
     */
    void setFdSignal(IasFdSignal *fdSignal, IasDeviceType deviceType);

    friend inline bool operator==( IasAudioRingBufferReal const & left, IasAudioRingBufferReal const & right);

    friend inline bool operator!=( IasAudioRingBufferReal const & left, IasAudioRingBufferReal const & right);

  private:

    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasAudioRingBufferReal(IasAudioRingBufferReal const &other);

    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasAudioRingBufferReal& operator=(IasAudioRingBufferReal const &other);

    uint32_t                                           mPeriodSize;       //!< period size in samples
    uint32_t                                           mNumPeriods;       //!< number of periods that fit in the buffer
    uint32_t                                           mNumChannels;      //!< number of channels per period
    uint32_t                                           mNumChannelsMax;   //!< the maximum supported number of channels
    uint32_t                                           mReadOffset;       //!< readOffset, similar to Alsa (frames)
    uint32_t                                           mWriteOffset;      //!< writeOffset, similar to Alsa (frames)
    IasAudioCommonDataFormat                              mDataFormat;       //!< the data format of the PCM samples
    int32_t                                            mSampleSize;       //!< number of bytes per sample
    uint32_t                                           mBufferLevel;      //!< fill level in samples
    bool                                             mShared;           //!< flag to indicate if the buffer is in shared memory
    bool                                             mInitialized;      //!< this flag is true when init function was successful
    std::atomic<bool>                                     mReadInProgress;
    std::atomic<bool>                                     mWriteInProgress;
    boost::interprocess::offset_ptr<void>                 mDataBuf;          //!< the offset pointer to the data memory
    IasIntProcMutex                                       mMutex;
    IasIntProcMutex                                       mMutexReadInProgress;  //!< to avoid that reset is executed while reading from buffer
    IasIntProcMutex                                       mMutexWriteInProgress; //!< to avoid that reset is executed while writing into buffer
    IasIntProcCondVar                                     mCondRead;
    IasIntProcCondVar                                     mCondWrite;
    uint32_t                                           mReadWaitLevel;
    uint32_t                                           mWriteWaitLevel;
    IasAudioTimestamp                                     mAudioTimestampAccessRead;  //!< AudioTimestamp of the last read access to the buffer
    IasAudioTimestamp                                     mAudioTimestampAccessWrite; //!< AudioTimestamp of the last write access to the buffer
    IasAudioRingBufferStreamingState                      mStreamingState;            //!< straming state: running, stopWrite, stopRead
    IasFdSignal                                          *mFdSignal;          //!< Signal based on filedescriptors between SmartXbar and user application
    IasDeviceType                                         mDeviceType;        //!< The device type to decide, when to trigger a signal
    uint32_t                                           mAvailMin;
};

inline bool operator==( IasAudioRingBufferReal const & left, IasAudioRingBufferReal const & right)
{

  if(left.mDataFormat != right.mDataFormat)
  {
    return false;
  }
  if(left.mNumChannels != right.mNumChannels)
  {
    return false;
  }
  if(left.mPeriodSize != right.mPeriodSize)
  {
    return false;
  }

  return true;
}

inline bool operator!=( IasAudioRingBufferReal const & left, IasAudioRingBufferReal const & right)
{
  if( (left.mDataFormat  != right.mDataFormat)  ||
      (left.mNumChannels != right.mNumChannels) ||
      (left.mPeriodSize  != right.mPeriodSize) )
  {
    return true;
  }
  else
  {
    return false;
  }
}

}
#endif
