/*
 * @COPYRIGHT_TAG@
 */
/**
 * @file
 */

#ifndef IASAUDIORINGBUFFERMIRROR_HPP_
#define IASAUDIORINGBUFFERMIRROR_HPP_

#include <atomic>
#include "audio/common/IasAudioCommonTypes.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"
#include "internal/audio/common/audiobuffer/IasAudioRingBufferResult.hpp"

// Forward declaration of snd_pcm_t (usually defined in alsa/asoundlib.h).
typedef struct _snd_pcm snd_pcm_t;


/*!
 * @brief namespace IasAudio
 */
namespace IasAudio {


/*!
 * @brief Documentation for class IasAudioRingBufferMirror
 */
class __attribute__ ((visibility ("default"))) IasAudioRingBufferMirror
{
  public:

    /*!
     *  @brief Constructor.
     */
    IasAudioRingBufferMirror();

    /*!
     *  @brief Destructor, virtual by default.
     */
    virtual ~IasAudioRingBufferMirror();

    IasAudioRingBufferResult init(uint32_t numChannels);

    IasAudioRingBufferResult setDeviceHandle(void* handle, uint32_t periodSize, uint32_t timeout_ms);
    void clearDeviceHandle();

    void setNonBlockMode(bool isNonBlocking);

    IasAudioRingBufferResult updateAvailable(IasRingBufferAccess access, uint32_t* samples);

    IasAudioRingBufferResult beginAccess(IasAudioArea* area, uint32_t* offset, uint32_t* frames);

    IasAudioRingBufferResult endAccess(uint32_t offset, uint32_t frames);

    IasAudioRingBufferResult getDataFormat(IasAudioCommonDataFormat *dataFormat) const;

    IasAudioRingBufferResult getTimestamp(IasAudioTimestamp *audioTimestamp) const;

    /**
     * @brief Start the ALSA sink device explicitly
     *
     * This method is required to explicitly start an ALSA sink device when it was prefilled with a specific
     * amount of zeros, but with space for one or more full periods.
     *
     * If less than one period size space is left, than the method updateAvailable automatically starts the ALSA
     * sink device.
     */
    void startDevice() const;

  private:
    /*!
     *  @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasAudioRingBufferMirror(IasAudioRingBufferMirror const &other);

    /*!
     *  @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasAudioRingBufferMirror& operator=(IasAudioRingBufferMirror const &other);

    int xrunRecovery(int err);

    snd_pcm_t*         mDevice;
    uint32_t           mNumChannels;
    bool               mInitialized;
    bool               mDeviceIsSet;
    std::atomic<bool>  mAccessInProgress;
    bool               mFirstLoop;            //!< Flag, becomes false after first frame
    uint32_t           mPeriodSize;           //!< Period size, expressed in PCM frames
    int32_t            mTimeout_ms;           //!< Timeout [ms] for snd_pcm_wait, or -1 for infinity
    bool               mNonBlockMode;         //!< True if updateAvailable() shall not wait for free frames in output buffer
    uint64_t           mNumTransmittedFrames; //!< Number of transmitted samples since start.
    IasAudioTimestamp  mAudioTimestamp;       //!< Current audio timestamp (pair of timestamp and numTransmittedFrames)
    DltContext        *mLog;                  //!< The DLT log context
};

} // namespace IasAudio

#endif // SOURCES__AUDIO_COMMON_PRIVATE_SRC_AUDIOBUFFER_IASAUDIORINGBUFFERMIRROR_HPP_
