/*
 * @COPYRIGHT_TAG@
 */
/**
 * @file   IasAudioRingBufferResult.hpp
 * @date   2015
 * @brief
 */

#ifndef IASAUDIORINGBUFFERRESULT_HPP_
#define IASAUDIORINGBUFFERRESULT_HPP_

#include "audio/common/IasAudioCommonTypes.hpp"

namespace IasAudio {

enum IasAudioRingBufferResult
{
  eIasRingBuffOk = 0,             //!< Success
  eIasRingBuffInvalidParam,       //!< Invalid parameter
  eIasRingBuffNotInitialized,     //!< Not initialized
  eIasRingBuffAlsaError,          //!< general ALSA error
  eIasRingBuffAlsaXrunError,      //!< XRUN recovery failed
  eIasRingBuffAlsaSuspendError,   //!< SUSPEND recovery failed
  eIasRingBuffNotAllowed,         //!< Not allowed here
  eIasRingBuffInvalidSampleSize,  //!< Invalid sample size
  eIasRingBuffTimeOut,            //!< timeout exceeded
  eIasRingBuffProbeError,         //!< Probe error
  eIasRingBuffCondWaitFailed,     //!< Cond wait failed
};

std::string toString(const IasAudioRingBufferResult&  type);

}
#endif
