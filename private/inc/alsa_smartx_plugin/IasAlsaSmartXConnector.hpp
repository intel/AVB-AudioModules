/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAlsaSmartXConnector.hpp
 * @date   20 Sep. 2015
 * @brief  Connector that connects the Alsa callback functions with the
 * provided smartx ring buffers
 */

#ifndef IAS_ALSASMARTXCONNECTOR_HPP_
#define IAS_ALSASMARTXCONNECTOR_HPP_

/*
 * ALSA
 */
#include <alsa/asoundlib.h>
#include <alsa/pcm_external.h>

/*
 * Ias
 */


/*
 * SmartX Plugin
 */
#include "internal/audio/common/alsa_smartx_plugin/IasAlsaPluginShmConnection.hpp"
#include "internal/audio/common/alsa_smartx_plugin/IasSmartXPluginIpcStructures.hpp"
#include "internal/audio/common/IasFdSignal.hpp"


namespace IasAudio {

class IasAlsaPluginShmConnection;

/**
 * @brief Forward declaration, will be used in the source file.
 */
class IasAudioIpc;

class IasAlsaSmartXConnector
{
  public:
    /**
     *  Constructor inits pointers with nullptr.
     */
    explicit IasAlsaSmartXConnector();

    /**
     *  Deletes the IO Plug structure if still not freed
     */
    ~IasAlsaSmartXConnector();

    /**
     * @brief Enum values to specifiy the transfer direction.
     *
     */
    enum TransferDirection {
      eIasCaptureTransfer,
      eIasPlaybackTransfer
    };

    /**
     * Initiates the ioplug structure inside the IasAlsaSmartXConnector class and sets the callback
     * structure to the static class member functions according to the stream type.
     * Will print errors on SNDERR(...).
     *
     * @return UNIX Error Code
     */
    int init(const char* name, const snd_pcm_stream_t& stream, const int& mode);

    /*
     * Public functions
     */

    /**
     * Function to evaluate the configuration of the device.
     *
     * @param conf Configuration structure from ALSA
     * @return UNIX Error Code. Zero is good.
     */
    int loadConfig(const snd_config_t* conf);

    /**
     * This function will return a Pointer to the PCM Handle
     * that is needed and returned by the exported plug in function.
     *
     * @return Pointer to a PCM Handle.
     */
    inline snd_pcm_t* getPCMHandle()
    {
      return mAlsaIoPlugData->pcm;
    }

    /**
     * Allocates the stream.
     *
     * @return
     */
    int prepareStreamConnection();

    /**
     * @brief Gets the path delay from the smart crossbar.
     *
     * @param[out] frames Delay in frames.
     * @return int UNIX Errorcode, zero is ok.
     */
    int getPathDelay(snd_pcm_sframes_t* frames);

    /**
     * @brief Set the hardware parameters.
     *
     * @param[out] params Current parameters.
     * @return int UNIX Errorcode, zero is good.
     */
    int setHwParams(snd_pcm_hw_params_t* params);

    /**
     * @brief Set the sw parameters.
     *
     * @param[in] params Current parameters.
     * @return int UNIX Errorcode, zero is good.
     */
    int setSwParams(snd_pcm_sw_params_t *params);

    /**
     * @brief Starts the playback and tells the smartx that the playback was started
     *
     * @return int UNIX Errorcode, zero is good.
     */
    int startStream();

    /**
     * @brief Stops the playback and tells the smartx that the playback was stopped.
     * Flush buffers.
     *
     * @return int UNIX Errorcode, zero is good.
     */
    int stopStream();

    /**
     * @brief Drains the remaining samples and stops the stream after.
     *
     * @return int UNIX Errorcode, zero is good.
     */
    int drainStream();

    /**
     * @brief Set the REvent of the given file descriptors. There is only one file descriptor and the Event
     * depends on the stream direction. (Playback and Capture). The function is called by the revent callback.
     *
     * @param pfd File descriptor from the smartx.
     * @param nfds File descriptor count.
     * @param[out] revents Flag which can be POLLIN or POLLOUT
     * @return int
     */
    int handlePollREvents(struct pollfd *pfd, unsigned int nfds, unsigned short* revents );

    /**
     * @brief Gets the current position in the buffer.
     *
     * @return snd_pcm_sframes_t Negative means error. Represent the current pointer position.
     */
    snd_pcm_sframes_t getFramePointer();

    /**
     * @brief Performs the copy form the Alsa buffer to the shared memory buffer or from shared memory to the
     * Alsa buffer. This depends on the direction parameter.
     *
     * @param areas Alsa data areas.
     * @param offset Offset in this area.
     * @param size Size of the transfer.
     * @param direction Direction Capture or Playback.
     * @return snd_pcm_sframes_t Number of frames transfered. Negative is an UNIX Error Code.
     */
    snd_pcm_sframes_t transferJob(const snd_pcm_channel_area_t* areas, snd_pcm_uframes_t offset,
                                  snd_pcm_uframes_t size , const TransferDirection& direction);

    /**
     * @brief Get the real number of available frames of the buffer
     *
     * For playback devices, this returns the number of free frames available in the buffer to fill by writing audio data.
     * For capture devices, this returns the number of audio frames available in the buffer to read.
     *
     * @return The real number of available frames of the buffer
     */
    snd_pcm_sframes_t getRealAvail();


    /*
     *  ALSA Static Callback Section
     */
    /**
     * Callback that calles the internal prepareStreamConnection function.
     *
     * @param io Pointer to the io plug data and the contained private data.
     * @return UNIX Error Code, Zero is good.
     */
    static int snd_pcm_smartx_prepare(snd_pcm_ioplug_t *io);

    /**
     * @brief Callback that calles the internal startStream function
     *
     * @param io Pointer to the io plug data and the contained private data.
     * @return UNIX Error Code, Zero is good.
     */
    static int snd_pcm_smartx_start(snd_pcm_ioplug_t *io);

    /**
     * @brief Callback that calles the internal stopStream function
     *
     * @param io Pointer to the io plug data and the contained private data.
     * @return UNIX Error Code, Zero is good.
     */
    static int snd_pcm_smartx_stop(snd_pcm_ioplug_t *io);

    /**
     * @brief Callback that calles the internal drainStream function
     *
     * @param io Pointer to the io plug data and the contained private data.
     * @return UNIX Error Code, Zero is good.
     */
    static int snd_pcm_smartx_drain(snd_pcm_ioplug_t *io);

    /**
     * @brief Callback that calles the internal close function
     *
     * @param io Pointer to the io plug data and the contained private data.
     * @return UNIX Error Code, Zero is good.
     */
    static int snd_pcm_smartx_close(snd_pcm_ioplug_t *io);

    /**
     * @brief Will return the current sample pointer.
     *
     * @param io Pointer to the io plug data and the contained private data.
     * @return snd_pcm_sframes_t Current sample pointer.
     * @return snd_pcm_sframes_t Negative is an error.
     */
    static snd_pcm_sframes_t snd_pcm_smartx_pointer(snd_pcm_ioplug_t *io);

    /**
     * @brief Callback that calls the internal transferJob function.
     *
     * @param io Pointer to the io plug data and the contained private data.
     * @param areas Channel areas where the playback data lays
     * @param offset Offset in the areas.
     * @param size Maximal size.
     * @return snd_pcm_sframes_t Number of correct transfered frames.
     */
    static snd_pcm_sframes_t snd_pcm_smartx_playback_transfer(snd_pcm_ioplug_t *io,
                                                     const snd_pcm_channel_area_t *areas,
                                                     snd_pcm_uframes_t offset,
                                                     snd_pcm_uframes_t size);

    /**
     * @brief Callback that calls the internal transferJob function.
     *
     * @param io Pointer to the io plug data and the contained private data.
     * @param areas Channel areas where the buffers are.
     * @param offset Offset in the areas.
     * @param size Maximal size.
     * @return snd_pcm_sframes_t Number of correct transfered frames.
     */
    static snd_pcm_sframes_t snd_pcm_smartx_capture_transfer(snd_pcm_ioplug_t *io,
                                                     const snd_pcm_channel_area_t *areas,
                                                     snd_pcm_uframes_t offset,
                                                     snd_pcm_uframes_t size);

    /**
     * @brief Callback function that calls the public setHwParams function.
     *
     * @param io Pointer to the io plug data and the contained private data.
     * @param params
     * @return int UNIX Error Code, Zero is good.
     */
    static int snd_pcm_smartx_hw_params(snd_pcm_ioplug_t *io, snd_pcm_hw_params_t *params);

    /**
     * @brief Callback function that calls the public setSwParams function.
     *
     * @param io Pointer to the io plug data and the contained private data.
     * @param params
     * @return int UNIX Error Code, Zero is good.
     */
    static int snd_pcm_smartx_sw_params(snd_pcm_ioplug_t *io, snd_pcm_sw_params_t *params);

    /**
     * @brief Callback function that calls internal handlePollREvents.
     *
     * @param io Pointer to the io plug data and the contained private data.
     * @param pfd ...
     * @param nfds ...
     * @param revents ...
     * @return int UNIX Error Code, Zero is good.
     */
    static int snd_pcm_smartx_poll_revents(snd_pcm_ioplug_t *io, struct pollfd *pfd,
                                           unsigned int nfds, unsigned short *revents);

    /**
     * @brief Callback function that calls the public getPathDelay function.
     *
     * @param io Pointer to the io plug data and the contained private data.
     * @param[out] frames Delay in frames.
     * @return int UNIX Error Code, Zero is good.
     */
    static int snd_pcm_smartx_delay(snd_pcm_ioplug_t *io, snd_pcm_sframes_t* frames);

    /**
     * @brief Callback function that calls the public getRealAvail function.
     *
     * @param[in] io Pointer to the io plug data and the contained private data.
     * @return The real available number of frames of the buffer.
     */
    static snd_pcm_sframes_t snd_pcm_smartx_real_avail(snd_pcm_ioplug_t *io);

  private:
    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasAlsaSmartXConnector(IasAlsaSmartXConnector const &other);

    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasAlsaSmartXConnector& operator=(IasAlsaSmartXConnector const &other);

    /**
     * This function fills the internal alsaCallback structure with the callbacks according to the stream type.
     * @param[in] stream Stream type (Playback/Capture)
     */
    void initCallbacks(const snd_pcm_stream_t& stream);

    /**
     * Function is searching for the Audio Buffer and stores the pointer in the class.
     *
     * @return UNIX Error Code
     */
    int connectToSmartX();

    /**
     * Function defines the HW Constraints out of the parameter buffer, that is stored in the
     * shared memory.
     *
     * @return UNIX Error Code
     */
    int defineHwConstraints();

    /**
     * Function to close the open once file
     *
     * This is used for clean-up purposes.
     */
    void closeOpenOnceFile();

    DltContext *mLog;                                 //!< DLT log context
    std::string mConnectionName;                      //!< Name of the device and the buffer that will be searched for.
    std::string mFullName;                            //!< Name of the device including the prefix (smartx_ or avb_)
    IasAlsaPluginShmConnection* mSmartxConnection;    //!< Connection Class with Pointers to the related shared memory. Will be available after the prepare function.
    IasAudioCurrentSetParameters mSetParams;          //!< Structure contains the set parameter and will be send to the smartx.
    snd_pcm_ioplug_t* mAlsaIoPlugData;                //!< Plugin SDK Data
    int32_t mNotificationDescriptor;               //!< The notification file descriptor that will be used for poll mechanism of the alsa plugin (deprecated, left in for binary compatibility)
    snd_pcm_ioplug_callback_t mAlsaCallbacks;         //!< Pointer to the callback structure that will be passed to alsa.
    IasAudioArea* mShmAreas;                          //!< Pointer to the local temporary areas for the transfer.
    snd_pcm_channel_area_t* mAlsaTransferAreas;       //!< Same as Shm Areas, converted for Alsa.
    uint32_t mTimeout;                             //!< Timeout value in msec for blocked read/write.
    snd_pcm_uframes_t mHwPtr;                         //!< The hardware pointer of the ALSA device (deprecated, left in for binary compatibility)
    snd_pcm_uframes_t mAvailMin;                      //!< Available minimum samples/free space
    uint32_t mRest;                                //!< Number of frames missing during last transfer to complete one period
    IasFdSignal mFdSignal;                            //!< Signal based on filedescriptors between SmartXbar and user application
    int mOpenOnceFd;                                  //!< File descriptor of the open once lock file
};

}

#endif // IAS_ALSASMARTXCONNECTOR_HPP_
