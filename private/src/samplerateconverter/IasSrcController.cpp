/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file    IasSrcController.cpp
 * @brief   Closed loop controller for adapting the conversion ratio
 *          of an asynchronous sample rate converter (ASRC).
 * @date    2015
 *
 * Closed loop controller for adapting the sample rate conversion ratio
 * in order to compensate for the clock skew between the input clock
 * domain and the output clock domain.
 *
 * The combination of this component and the core sample rate converter
 * IasSrcFarrow results in an asynchronous sample rare converter (ASRC).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "internal/audio/common/samplerateconverter/IasSrcController.hpp"

#ifdef __linux__
#define MS_VC  0
#else
#define MS_VC  1
#endif


namespace IasAudio {

static const IasSrcControllerConfigParams cConfigParamsDefault = {
  0.008f,    // mKp             proportional action coefficient ("Proportionalbeiwert")
  300.0f,    // mTn = Tn / Tb   resert time ("Nachstellzeit") / Blocklength
  0.9f,      // mCoeffLowPass
  0.9f,      // mRatioMin
  1.1f,      // mRatioMax
};


/**
 * @brief Constructor
 *
 * All member variables that are not initialized by the constructor will be
 * initialized by the methods init() and reset(). The flag mIsInitialized
 * assures that the init() and reset() methods must be executed before the
 * process() method can be used.
 */
IasSrcController::IasSrcController()
  :mIsInitialized(false)
  ,mRatioAdaptive(0.0f)        // the actual initializations are done by the init() method
  ,mOutputLowPassOld(0.0f)
  ,mOutputController(0.0f)
  ,mOutputActive(false)
  ,mJitterBufferLength(0)      // the actual initializations are done by the setJitterBufferParams() method
  ,mJitterBufferTargetLevel(0)
  ,mInvJitterBufferTargetLevel(0)
{
  // Fill the configuration parameter struct with default parameters.
  mConfigParams = cConfigParamsDefault;
}


/**
 * @brief Destructor
 */
IasSrcController::~IasSrcController()
{
}


/**
 * @brief Init function
 */
IasSrcController::IasResult IasSrcController::init()
{
  if (mIsInitialized)
  {
    return eIasInitFailed;
  }

  // Fill the configuration parameter struct with default parameters.
  mConfigParams = cConfigParamsDefault;

  mIsInitialized  = true;

  // Call the reset function
  this->reset();

  return eIasOk;
}


/**
 * @brief Declare the parameters of the associated jitter buffer.
 */
IasSrcController::IasResult IasSrcController::setJitterBufferParams(uint32_t jitterBufferLength,
                                                                    uint32_t jitterBufferTargetLevel)
{
  if ((jitterBufferLength == 0) || (jitterBufferTargetLevel == 0))
  {
    return eIasInvalidParam;
  }

  mJitterBufferLength         = jitterBufferLength;
  mJitterBufferTargetLevel    = jitterBufferTargetLevel;
  mInvJitterBufferTargetLevel = 1.0f / static_cast<float>(jitterBufferTargetLevel);

  return eIasOk;
}


/**
 * @brief Reset function.
 */
void IasSrcController::reset()
{
  mRatioAdaptive     = 1.0f;
  mOutputLowPassOld  = 0.0f;
  mOutputController  = 0.0f;
  mOutputActive      = false;
}


/**
 * @brief Process the controller in order to update the adaptive conversion
 *        ratio. Furthermore, the controller updates the flag outputActive,
 *        which indicates whether or not the framework shall transmit samples
 *        from the jitter buffer to the output.
 */
IasSrcController::IasResult IasSrcController::process(float   *ratioAdaptive,
                                                      bool      *outputActive,
                                                      uint32_t     jitterBufferCurrentLevel)
{
  // Exit this function if it has not been initialized appropriately.
  if ((!mIsInitialized) ||
      (mJitterBufferLength      == 0) ||
      (mJitterBufferTargetLevel == 0))
  {
    return eIasNotInitialized;
  }

  // State machine for controlling mOutputActive depending on the current fill level
  if (jitterBufferCurrentLevel > mJitterBufferTargetLevel)
  {
    mOutputActive = true;
  }
  else if (jitterBufferCurrentLevel == 0)
  {
    mOutputActive = false;
  }

  if (mOutputActive)
  {
    // difference between the target fill level and current fill level
    float diff = (static_cast<float>(jitterBufferCurrentLevel) -
                         static_cast<float>(mJitterBufferTargetLevel)) * mInvJitterBufferTargetLevel;

    // output signal of the 1st order low-pass filter
    float outputLowPass = (1.0f - mConfigParams.mCoeffLowPass) * diff + mConfigParams.mCoeffLowPass * mOutputLowPassOld;

    // PI Controller, according to Latzel "Einführung in die Digitalen Regelungen", eq. (3.2.45)
    mOutputController = (mOutputController
                         + mConfigParams.mKp * (1.0f+0.5f/mConfigParams.mTn) * outputLowPass
                         - mConfigParams.mKp * (1.0f-0.5f/mConfigParams.mTn) * mOutputLowPassOld);

    mOutputLowPassOld = outputLowPass;
    mRatioAdaptive  = 1.0f + mOutputController;

    // Saturate mRatioAdaptive, such that the result is within the interval [mRatioMin...mRatioMax].
    mRatioAdaptive = std::min(mRatioAdaptive, mConfigParams.mRatioMax);
    mRatioAdaptive = std::max(mRatioAdaptive, mConfigParams.mRatioMin);
  }

  *ratioAdaptive = mRatioAdaptive;
  *outputActive  = mOutputActive;

  return eIasOk;
}


/*
 * Function to get a IasSrcController::IasResult as string.
 */
#define STRING_RETURN_CASE(name) case name: return std::string(#name); break
#define DEFAULT_STRING(name) default: return std::string(name)
std::string toString(const IasSrcController::IasResult& type)
{
  switch(type)
  {
    STRING_RETURN_CASE(IasSrcController::eIasOk);
    STRING_RETURN_CASE(IasSrcController::eIasInvalidParam);
    STRING_RETURN_CASE(IasSrcController::eIasInitFailed);
    STRING_RETURN_CASE(IasSrcController::eIasNotInitialized);
    DEFAULT_STRING("Invalid IasSrcController::IasResult => " + std::to_string(type));
  }
}


} // namespace IasAudio
