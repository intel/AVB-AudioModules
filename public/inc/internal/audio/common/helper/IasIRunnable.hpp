/*
@COPYRIGHT_TAG@
*/
/**
 * @file
 */

#ifndef _IAS_IRUNNABLE_HPP
#define _IAS_IRUNNABLE_HPP

#include "audio/common/IasAudioCommonTypes.hpp"

/**
 * @brief IasAudio
 */
namespace IasAudio
{

/**
 * @brief IRunnable interface for usage with the IasThread.
 */
class IAS_AUDIO_PUBLIC IasIRunnable
{
  public:

    /**
     * Constructor.
     */
    IasIRunnable();

    /**
     * Destructor.
     */
    virtual ~IasIRunnable();

    /**
     * Function called before the run function.
     *
     * @returns Value indicating success or failure.
     */
    virtual IasAudioCommonResult beforeRun() = 0;

    /**
     * The actual run function, does the processing.
     *
     * Stay inside the function until all processing is finished or shutDown is called.
     * If this call returns an error value, this error value is reported via the return value of \ref IasThread::start().
     * In case of an error, the thread still needs to be shutdown explicitly through calling \ref IasThread::stop().
     *
     * @returns Value indicating success or failure.
     *
     * @note This value can be accessed through \ref IasThread::getRunThreadResult.
     */
    virtual IasAudioCommonResult run() = 0;

    /**
     * ShutDown code,
     * called when thread is going to be terminated.
     *
     * Exit the \ref run function when this function is called.
     *
     * @returns Value indicating success or failure.
     *
     */
    virtual IasAudioCommonResult shutDown() = 0;

    /**
     * Function called after the run function.
     *
     * @returns Value indicating success or failure.
     *
     * If this call returns an error value and run() was successful, it is reported via the return value of IasThread::stop().
     */
    virtual IasAudioCommonResult afterRun() = 0;


};

}; //namespace IasAudio


#endif /* _IAS_IRUNNABLE_HPP_ */
