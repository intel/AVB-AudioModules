/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*!
 * @file IasAudioRingBuffer.hpp
 */

#ifndef IASAUDIORINGBUFFER_HPP_
#define IASAUDIORINGBUFFER_HPP_


#include "internal/audio/common/audiobuffer/IasAudioRingBufferTypes.hpp"
#include "internal/audio/common/audiobuffer/IasAudioRingBufferResult.hpp"
#include "audio/common/IasAudioCommonTypes.hpp"

#include "boost/interprocess/offset_ptr.hpp"
#include <map>

/*!
 * @brief namespace IasAudio
 */
namespace IasAudio {

class IasMetaData;
class IasDataProbe;
class IasAudioRingBufferReal;
class IasAudioRingBufferMirror;
class IasFdSignal;


/*!
 * @brief Documentation for class IasAudioRingBuffer
 */
class __attribute__ ((visibility ("default"))) IasAudioRingBuffer
{
  public:

    using IasDataProbePtr = std::shared_ptr<IasDataProbe>;

    using IasDataProbeMap = std::map<const std::string, IasDataProbePtr>;

    using IasDataProbeMapPair = std::pair<const std::string, IasDataProbePtr>;
    /*!
     *  @brief Constructor.
     */
    IasAudioRingBuffer();

    /*!
     *  @brief Destructor, virtual by default.
     */
    virtual ~IasAudioRingBuffer();

    /*!
     * @brief Initialize an audio ring buffer. The ring buffer will be of type eIasRingBufferShared or eIasRingBufferLocalReal,
     *        depending on the boolean parameter @a shared.
     *
     * @returns                    eIasRingBuffOk on success, otherwise an error code.
     *
     * @param[in]  nPeriodSize     Length of each period
     * @param[in]  nPeriods        Number of periods
     * @param[in]  nChannels       Number of channels
     * @param[in]  dataFormat      The data format of the ringbuffer ( Int16, Int32 or Float32)
     * @param[in]  dataBuf         The real data buffer
     * @param[in]  shared          Specifies whether the ring buffer will be of type eIasRingBufferShared or eIasRingBufferLocalReal
     * @param[in]  metaData        ...
     * @param[in]  ringbufReal     pointer to the IaAudioRingBufferReal
     */
    IasAudioRingBufferResult init(uint32_t periodSize,
                                  uint32_t nPeriods,
                                  uint32_t nChannels,
                                  IasAudioCommonDataFormat dataFormat,
                                  void* dataBuf,
                                  bool shared,
                                  IasMetaData* metaData,
                                  IasAudioRingBufferReal* ringbufReal);

    /*!
     * @brief Initialize an audio ring buffer. The ring buffer will be of type eIasRingBufferLocalMirror.
     *
     * @returns                    eIasRingBuffOk on success, otherwise an error code.
     *
     * @param[in]  nChannels       Number of channels
     * @param[in]  ringbufMirror   pointer to the IaAudioRingBufferMirror
     */
    IasAudioRingBufferResult init(uint32_t nChannels,
                                  IasAudioRingBufferMirror* ringbufMirror);

    /*!
     * @brief Setup a ringbuffer. This function is used by the rigbuffer factory when
     *        an IasAudioRingBufferReal was found in shared memory and a new created IasAudioRingBuffer
     *        needs to be setup finally, like setup areas etc.
     *
     * @returns                 eIasRingBuffOk on success, otherwise an error code.
     * @retval                  eIasRingBuffInvalidParam   null pointer passed as parameter
     * @retval                  eIasRingBuffNotInitialized ringbufReal was not initialized properly
     *
     * @param[in]  ringbufReal  pointer to IasAudioRingBufferReal
     */
    IasAudioRingBufferResult setup(IasAudioRingBufferReal* ringBufReal);

    /*!
     * @brief Register an ALSA device to a ring buffer that is of type @a eIasRingBufferLocalMirror.
     *
     * @returns               eIasRingBuffOk on success, otherwise an error code.
     *
     * @param[in]  handle     Handle for an ALSA device, must point to an object of type snd_pcm_t.
     * @param[in]  periodSize Period size, expressed in PCM frames. This must be equal to the period
     *                        size that has been declared to the ALSA device via the hardware params.
     * @param[in]  timeout_ms Timeout period, expressed in ms. The timeout is relevent for
     *                        the IasAudioRingBuffer::updateAvailable method.
     */
    IasAudioRingBufferResult setDeviceHandle(void* handle, uint32_t periodSize, uint32_t timeout_ms);

    /*!
     * @brief Clear the device handle registered previously via setDeviceHandle.
     */
    void clearDeviceHandle();

    /*!
     * @brief Let the ring buffer operate in non-blocking mode.
     *
     * This function is relevant only for ring buffers of type @a eIasRingBufferLocalMirror.
     * If the ring buffer operates in non-blocking mode, the IasAudioRingBuffer::updateAvailable
     * method does not wait until the number of frames becomes bigger than the periodSize.
     * The default behavior is blocking mode.
     *
     * @returns    error code
     * @retval     eIasRingBuffOk               on success
     * @retval     eIasRingBuffNotInitialized   component has not been initialized
     *
     * @param[in]  isNonBlocking                Defines whether the IasAudioRingBuffer::updateAvailable
     *                                          shall block until the number of frames becomes bigger than
     *                                          the periodSize.
     */
    IasAudioRingBufferResult setNonBlockMode(bool isNonBlocking);

    /*!
     * @brief Get the number of frames ready to be read (capture) / written (playback).
     *
     * If the ring buffer is of type @a eIasRingBufferLocalMirror, this function waits until the
     * number of frames becomes bigger than the periodSize, which has been declared by means of
     * the IasAudioRingBuffer::setDeviceHandle method. If the timeout period is elaped, this
     * function returns with an error code eIasRingBuffTimeOut.
     *
     * @returns    error code
     * @retval     eIasRingBuffOk               on success
     * @retval     eIasRingBuffInvalidParam     one of the parameters is not valid
     * @retval     eIasRingBuffNotInitialized   component has not been initialized
     * @retval     eIasRingBuffTimeOut          timeout occured (only for type @a eIasRingBufferLocalMirror)
     * @retval     eIasRingBuffAlsaXrunError    XRUN recovery failed (only for type @a eIasRingBufferLocalMirror)
     * @retval     eIasRingBuffAlsaSuspendError SUSPEND recovery failed (only for type @a eIasRingBufferLocalMirror)
     * @retval     eIasRingBuffAlsaError        other ALSA error (only for type @a eIasRingBufferLocalMirror)
     *
     * @param[in]  access   Specifies the access type (either eIasRingBufferAccessRead or eIasRingBufferAccessWrite).
     * @param[out] samples  Returned number of samples that are ready to be processed.
     */
    IasAudioRingBufferResult updateAvailable(IasRingBufferAccess access, uint32_t* samples);

    /*!
     * @brief Request to access a portio
     *
     * n of an mmap'ed area.
     *
     * The function should be called before a direct (mmap) area can be accessed.
     * The resulting size parameter is always less or equal to the input count of
     * frames and can be zero, if no frames can be processed, i.e., if the the ring
     * buffer is full (playback) or empty (capture).
     *
     * It is necessary to call the IasAudioRingBuffer::updateAvailable() method directly before this call.
     *
     * @returns       error code
     * @retval        eIasRingBuffOk               on success
     * @retval        eIasRingBuffInvalidParam     one of the parameters is not valid
     * @retval        eIasRingBuffNotInitialized   component has not been initialized
     * @retval        eIasRingBuffNotAllowed       access is already in progress
     * @retval        eIasRingBuffAlsaError        ALSA error (only for type @a eIasRingBufferLocalMirror)
     *
     * @param[in]     access  Specifies the access type (either eIasRingBufferAccessRead or eIasRingBufferAccessWrite).
     * @param[out]    area    Returned mmap areas (one area for each channel), use IasAudioRingBuffer::getAreas()
     *                        to get a pointer to an appropriate vector.
     * @param[out]    offset  Returned mmap area offset in area steps (== frames).
     * @param[in,out] frames  mmap area portion size in frames (wanted on entry, contiguous available on exit).
     */
    IasAudioRingBufferResult beginAccess(IasRingBufferAccess access, IasAudioArea** area, uint32_t* offset, uint32_t* frames);

    /*!
     * @brief Declare that we have finished accessing a portion of an mmap'ed area.
     *
     * You should pass this function the offset value that IasAudioRingBuffer::beginAccess()
     * returned. The frames parameter should hold the number of frames you have written or read
     * to/from the audio buffer. The frames parameter must never exceed the contiguous frames
     * count that IasAudioRingBuffer::beginAccess() returned.
     *
     * Each call of IasAudioRingBuffer::beginAccess() must be followed by a call of
     * IasAudioRingBuffer::endAccess().
     *
     * @returns   error code
     * @retval    eIasRingBuffOk               on success
     * @retval    eIasRingBuffInvalidParam     one of the parameters is not valid
     * @retval    eIasRingBuffNotInitialized   component has not been initialized
     * @retval    eIasRingBuffNotAllowed       access is already in progress
     * @retval    eIasRingBuffAlsaError        ALSA error (only for type @a eIasRingBufferLocalMirror)
     *
     * @param[in] access  Specifies the access type (either eIasRingBufferAccessRead or eIasRingBufferAccessWrite).
     * @param[in] offset  Offset in area steps (== frames), must be equal to the offset value that
     *                    IasAudioRingBuffer::beginAccess() returned.
     * @param[in] frames  mmap area portion size in frames (== number of frames that we have processed)
     */
    IasAudioRingBufferResult endAccess(IasRingBufferAccess access, uint32_t offset, uint32_t frames);

    /*!
     * @brief Trigger the associated IasFdSignal.
     */
    void triggerFdSignal();

    /*!
     * @brief Get a pointer to the ring buffer areas. Only allowed for IasAudioRingBufferReal, because there the
     *        areas are managed by the ringbuffer itself. For an IasAudioRingBufferMirror, the areas come directly from ALSA
     *
     * @returns                 eIasRingBuffOk on success, otherwise an error code.
     * @retval                  eIasRingBuffNotAllowed function is called for a mirror buffer
     *
     * @param[in,out] areas     Returned pointer to the ring buffer areas
     */
    IasAudioRingBufferResult getAreas(IasAudioArea** areas) const;

    /*!
     * @brief Get the data format that is used for representing the PCM samples.
     *
     * @returns                 eIasRingBuffOk on success, otherwise an error code.
     *
     * @param[out] dataFormat   Returned data format
     */
    IasAudioRingBufferResult getDataFormat(IasAudioCommonDataFormat *dataFormat) const;

    /*!
     * @brief function to read from ringbuffer (with timeout) when a desired buffer level is reached.
     *        the function either returns when a timeout occurs or when the level is reached.
     *
     * @returns                 error code
     * @retval                  eIasRingBuffOk            success
     * @retval                  eIasRingBuffInvalidParam  params not valid
     * @retval                  eIasRingBuffTimeOut       function timed out, buffer level was not reached within timeout
     *
     * @param[in] timeout_ms          timeout in ms, function will return if buffer level is not reached within timeout, must be > 0
     * @param[in] numPeriods          the desired buffer level, must be > 0 and >= total buffer size
     */
    IasAudioRingBufferResult waitRead(uint32_t numPeriods, uint32_t timeout_ms);

    /*!
     * @brief function to write to ringbuffer (with timeout) when desired buffer space is available.
     *
     * @returns                 error code
     * @retval                  eIasRingBuffOk            success
     * @retval                  eIasRingBuffInvalidParam  params not valid
     * @retval                  eIasRingBuffTimeOut       function timed out, buffer level was not reached within timeout
     *
     * @param[in] timeout_ms          timeout in ms, function will return if buffer level is not reached within timeout, must be > 0
     * @param[in] numPeriods          the desired space in buffer, must be > 0 and >= total buffer size
     */
    IasAudioRingBufferResult waitWrite(uint32_t numPeriods, uint32_t timeout_ms);

    /*!
     * @brief function to return the number of channels
     *
     * @returns the number of channels
     *
     */
    uint32_t getNumChannels() const {return mNumChannels;};

    /*!
     * @brief function to return the pointer to the real buffer
     *
     * @returns pointer to real buffer, which is const
     *
     */
    const IasAudioRingBufferReal* getReal() const {return mRingBufReal;};

    /*!
     * @brief function to return the pointer to the mirror buffer
     *
     * @returns pointer to mirror buffer, which is const
     *
     */
    const IasAudioRingBufferMirror* getMirror() const {return mRingBufMirror;};

    /*!
     * @brief Return the audioTimestamp of the latest buffer transfer.
     *
     * The IasAudioTimestamp represents a pair consisting of
     *
     * @li the timestamp specifying when the buffer was transferred
     * @li the number of PCM frames that have been transferred since the system has been started.
     *
     * If the buffer is a mirror buffer, the audioTimestamp corresponds to the last transfer of
     * PCM frames that has been done by the ALSA device. The paremeter @a access is ignored in this case.
     *
     * If the buffer is a real buffer, the audioTimestamp corresponds to the last read access or
     * write access, depending on the parameter @a access.

     * @returns  error code.
     * @retval   eIasRingBuffOk            Operation was successful.
     * @retval   eIasRingBuffInvalidParam  The parameter timestamp was nullptr.
     *
     * @param[in]  access          Specifies to what buffer access type (eIasRingBufferAccessRead or
     *                             eIasRingBufferAccessWrite) the timestamp shall refer to.
     *                             This parameter is ignored if the buffer is a mirror buffer.
     * @param[out] audioTimestamp  Returns the audioTimestamp (pair of timestamp and numTransmittedFrames).
     *                             The member audioTimestamp.timestamp represents the micoseconds since epoch time.
     */
    IasAudioRingBufferResult getTimestamp(IasRingBufferAccess access, IasAudioTimestamp *audioTimestamp) const;

    /*!
     * @brief Set the streaming state of the ring buffer (supported only for real buffers).
     *
     * Supported streaming states are:
     *
     * @li eIasRingBuffStreamingStateRunning     it is possible to write into and to write from the buffer
     * @li eIasRingBuffStreamingStateStopWrite   write accesses are blocked (PCM frames are discarded)
     * @li eIasRingBuffStreamingStateStopRead    read accesses are blocked (no PCM frames are provided)
     *
     * @param[in] streamingState            The streaming state.
     * @returns                             Error code.
     * @retval    eIasRingBuffOk            Operation was successful.
     * @retval    eIasRingBuffNotAllowed    The streaming state cannot be set if the ring buffer is a mirror buffer.
     */
    IasAudioRingBufferResult setStreamingState(IasAudioRingBufferStreamingState streamingState);

    /*!
     * @brief   Get the streaming state of the ring buffer.
     * @returns The streaming state, or eIasRingBuffStreamingStateUndefined if the ring buffer is a mirror buffer.
     */
    IasAudioRingBufferStreamingState getStreamingState() const;

    /*!
     * @brief   Get the read offset (index within the ring buffer). Supported only for real buffers.
     * @returns The read offset.
     */
    uint32_t getReadOffset() const;

    /*!
     * @brief   Get the write offset (index within the ring buffer).  Supported only for real buffers.
     * @returns The write offset.
     */
    uint32_t getWriteOffset() const;

    /*!
     * @brief Get the continuously increasing hw ptr for read access.
     *
     * This is required for the ALSA io-plug to correctly calculate its own hw ptr.
     * Supported only for real buffers.
     * @returns The hw ptr for read access.
     */
    int64_t getHwPtrRead() const;

    /*!
     * @brief Get the continuously increasing hw ptr for write access.
     *
     * This is required for the ALSA io-plug to correctly calculate its own hw ptr.
     * Supported only for real buffers.
     * @returns The hw ptr for read access.
     */
    int64_t getHwPtrWrite() const;

    /*!
     * @brief Reset the readOffset and the writeOffset to zero, so that the ring buffer will be empty again.
     *
     * The function is intended to be called by the writer thread, while there is no write access in progress.
     * The function applies a mutex, so that the readOffset is not modified while the reader thread reads
     * from the buffer.
     *
     * This function is supported only for real buffers.
     */
    void resetFromWriter();

    /*!
     * @brief Reset the readOffset and the writeOffset to zero, so that the ring buffer will be empty again.
     *
     * The function is intended to be called by the reader thread, while there is no read access in progress.
     * The function applies a mutex, so that the writeOffset is not modified while the writer thread writes
     * into the buffer.
     *
     * This function is supported only for real buffers.
     */
    void resetFromReader();

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
     * This method is only relevant for real buffers.
     *
     * @param[in] availMin  The minimum number of frames that must be available before the fdSignal might be triggered.
     */
    void setAvailMin(uint32_t availMin);

    /*!
     * @brief Set the boundary value
     *
     * The boundary value is used as a wrap around point to avoid overflow of the snd_pcm_sframes_t range.
     *
     * @param[in] boundary The boundary value as defined in the ALSA pcm device
     */
    void setBoundary(uint64_t boundary);

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

    /**
     * @brief Set the name of the ring buffer
     *
     * @param[in] name The name of the ring buffer
     */
    void setName(const std::string &name);

    /**
     * @brief Get the name of the ring buffer
     *
     * @returns The name of the ring buffer
     */
    const std::string& getName() const;

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

  private:
    /*!
     *  @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasAudioRingBuffer(IasAudioRingBuffer const &other);

    /*!
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasAudioRingBuffer& operator=(IasAudioRingBuffer const &other);

    IasAudioRingBufferReal    *mRingBufReal;       //!< pointer to IasAudioRingBufferReal
    IasAudioRingBufferMirror  *mRingBufMirror;     //!< pointer to IasAudioRingBufferMirror
    IasAudioArea              *mAreas;             //!< pointer to the IasAudioAreas
    bool                       mReal;              //!< flag to indicate if it is a mirror or real buffer
    uint32_t                   mNumChannels;       //!< the number of channels
    std::string                mName;              //!< the name of the ring buffer
};

} // namespace Ias

#endif // IASAUDIORINGBUFFER_HPP_
