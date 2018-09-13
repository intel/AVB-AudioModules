/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file    IasSrcController.hpp
 * @brief   Closed loop controller for adapting the conversion ratio
 *          of an asynchronous sample rate converter (ASRC).
 * @date    2015
 *
 * The ASRC controller adapts the conversion ratio in order to
 * compensate for the clock skew between the input clock domain and
 * the output clock domain.

 * The combination of this component and the core sample
 * rate converter IasSrcFarrow results in an asynchronous
 * sample rare converter.
 */

#ifndef IASSRCCONTROLLER_HPP_
#define IASSRCCONTROLLER_HPP_

#include "audio/common/IasAudioCommonTypes.hpp"

namespace IasAudio {

/*!
 * @brief Struct with all configuration parameters of the ASRC controller.
 *
 * A detailled description for these parameters can be found in the document
 */
typedef struct {
  float  mKp;            //!< proportional action coefficient ("Proportionalbeiwert")
  float  mTn;            //!< resert time ("Nachstellzeit") / Blocklength
  float  mCoeffLowPass;  //!< recursive coefficient (a1) of the 1st order low-pass filter
  float  mRatioMin;      //!< lower limit for the adaptive conversion ratio
  float  mRatioMax;      //!< upper limit for the adaptive conversion ratio
} IasSrcControllerConfigParams;


/*****************************************************************************
 * @brief Class IasSrcController
 *****************************************************************************
 */


class __attribute__ ((visibility ("default"))) IasSrcController
{
  public:

    /**
     * @brief  Result type of the class IasSrcController.
     */
    enum IasResult
    {
      eIasOk,               //!< Ok, Operation successful
      eIasInvalidParam,     //!< Invalid parameter, e.g., out of range or NULL pointer
      eIasInitFailed,       //!< Initialization of the component failed
      eIasNotInitialized,   //!< Component has not been initialized appropriately
      eIasFailed,           //!< other error
    };


    /*!
     * @brief Constructor.
     */
    IasSrcController();

    /*!
     * @brief Destructor.
     */
    ~IasSrcController();

    /*!
     * @brief Init function.
     *
     * @returns  Error code.
     * @retval   eIasOk          Initialization successful
     * @retval   eIasInitFailed  Initailization failed (component is already initialized)
     */
    IasResult init();


    /*!
     * @brief Declare the parameters of the associated jitter buffer.
     *
     * @returns  Error code.
     * @retval   eIasOk            Initialization successful
     * @retval   eIasInvalidParam  Invalid parameters (must not be zero)
     */
    IasResult setJitterBufferParams(uint32_t jitterBufferLength,
                                    uint32_t jitterBufferTargetLevel);

    /*!
     * @brief Reset function.
     */
    void reset();

    /*!
     * @brief Process function of the (A)SRC closed loop controller:
     *
     * This function executes the (A)SRC closed loop controller in order to update
     * the adaptive conversion ratio. Furthermore, the controller updates the flag
     * @a outputActive, which indicates whether or not the framework shall transmit
     * samples from the jitter buffer to the output.
     *
     * The adaptive conversion ratio, which is updated by this function, is normalized
     * to the nominal conversion ratio. A value of 1.0 means that the sample rate
     * converter shall apply the nominal conversion ratio. In general, we have:
     * \verbatim
     *
     * fs_in                                 fs_in_nominal
     * ------  =  conversionRatioAdaptive * -----------------
     * fs_out                                fs_ out_nominal
     *
     * \endverbatim
     *
     * @param[out] ratioAdaptive            Normalized conversion ratio.
     * @param[out] outputActive             Flag indicating whether samples shall be
     *                                      streamed from the jitter buffer to the output.
     *                                      If outputActive is false, dummy samples shall
     *                                      be generated instead.
     * @param[in]  jitterBufferCurrentLevel Number of samples that are currently buffered
     *                                      within the jitter buffer at the output of the
     *                                      asynchronous sample rate converter.
     *
     * @returns  Error code.
     * @retval   eIasOk              Operation successful
     * @retval   eIasNotInitialized  Component not initialized (missing call of init() or setJitterBufferParams())
     */
    IasResult process(float   *ratioAdaptive,
                      bool      *outputActive,
                      uint32_t     jitterBufferCurrentLevel);

  protected:
    /*
     * member variables
     */
    bool      mIsInitialized;

    float   mRatioAdaptive;    // conversion ratio, adapted by this module
    float   mOutputLowPassOld; // previous sample of the 1st order low-pass filter output signal
    float   mOutputController; // output signal of the PI controller

    bool      mOutputActive;

    uint32_t    mJitterBufferLength;
    uint32_t    mJitterBufferTargetLevel;
    float   mInvJitterBufferTargetLevel;    // 1.0 / mJitterBufferTargetLevel

    IasSrcControllerConfigParams mConfigParams;
};


/**
 * @brief Function to get a IasSrcController::IasResult as string.
 *
 * @return String carrying the result message.
 */
__attribute__ ((visibility ("default"))) std::string toString(const IasSrcController::IasResult& type);


} // namespace IasAudio

#endif // IASSRCCONTROLLER_HPP_
