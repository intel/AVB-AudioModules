/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file    IasThdnEstimator.hpp
 * @brief   Estimates THDN to provide reliable feedback
 *          regarding the quality of the examined signals.
 * @date    Dec 4, 2015
 */

#ifndef IAS_THDN_ESTIMATOR_HPP_
#define IAS_THDN_ESTIMATOR_HPP_

#include <emmintrin.h>
#include <iostream>
#include <fstream>
#include <malloc.h>
#include <math.h>
#include <string.h>
#include <vector>
#include <xmmintrin.h>


#include "smartx_test_support/IasFftFunctions.hpp"

namespace IasAudio {


/*!
 *  @class IasThdnEstimator
 *  @brief Class that provides functions to estimate the THDN of the
 *         examined signal for reliable feedback regarding the quality.
 */
class __attribute__ ((visibility ("default"))) IasThdnEstimator
{
public:

  /*!
   * @brief The return values of the methods.
   */
  enum IasResult
  {
    eIasOk = 0,                 //!< Success
    eIasInvalidParameter,       //!< Invalid FFT length, must be a power of 2 and >= 2.
    eIasInvalidNullPointer,     //!< Pointer to buffer must not be 'nullptr'
    eIasOutOfMemory,            //!< Out of Memory
    eIasInitFailed,             //!< Init of FFT function failed
    eIasAlreadyInitialized      //!< Already initialized
  };

  /*!
   *  @brief Constructor.
   */
  IasThdnEstimator();

  /*!
   *  @brief Destructor, virtual by default.
   */
  virtual ~IasThdnEstimator();

  /*!
   * @brief Initialization function, must be called after the class object has been created.
   *
   * @param [in] fftLength     Length of the fft. This value must be a power of two.
   *                           Calculated spectra have the size of (fft-length / 2 + 1) * 2.
   * @param [in] windowType    Window used for windowing the input signal before transforming
   *                           it into frequency domain.
   * @param [in] numChannels   Maximum number of channels that will be processed.
   * @return IasResult Error code. Returns eIasOk, if no error occured.
   * @retval eIasInvalidParameter   One of the given parameters exceeds given limits.
   * @retval eIasAlreadyInitialized The thdn estimator is already initialized.
   * @retval eIasOutOfMemory        Memalign went wrong, probably out of memory.
   * @retval eIasInvalidNullPointer Channel array pointer is nullptr.
   * @retval eIasInitFailed         Initialization of the IasFftFunctions object failed.
   * @retval eIasOk                 Initialized successfully.
   */
  IasResult init(uint32_t fftLength, uint32_t numChannels);

  /*!
   * @brief Process on frame of input data.
   *
   * @param [in] input         Real input data with the size of the fft-length.
   *                           Supported formats are 'float', 'int16_t' and
   *                           'int32_t'.
   * @param [in] newSamples    Number of samples added to the internal linear buffer.
   * @param [in] channel       Channel which will be processed.
   * @param [in] inputStride   For multichannel operation the input stride, which defines
   *                           the index distance between two adjecant samples of the same channel.
   * @return IasResult Error code. Returns eIasOk, if no error occured.
   * @retval eIasInitFailed       Thdn estimator has not yet been initialized.
   * @retval eIasInvalidParameter One of the parameters does exceed its limits.
   * @retval eIasOk               Processed successfully.
   */
  template <typename T>
  IasResult processFrame(T *input,
                         uint32_t newSamples,
                         float samplrate,
                         uint32_t channel,
                         uint32_t inputStride = 1);

  /*!
   * @brief Setup which sweep will be used for analysing purpose.
   *
   * @param [in] sweepStart    Frequency in [Hz] at which the sweep starts.
   * @param [in] sweepGradient Frequency gradient with which the sweep's frequency in [Hz]
   *                           rises / falls per sample.
   * @param [in] nyquist       The nyquist frequency of the signal fed into the estimator.
   * @param [in] channel       Channel which will be setup.
   * @return IasResult Error code. Returns eIasOk, if no error occured.
   * @retval eIasInitFailed       Thdn estimator has not yet been initialized.
   * @retval eIasInvalidParameter One of the parameters does exceed its limits.
   * @retval eIasOk               Setup succeeded.
   */
  IasResult setup(float sweepStart, float sweepGradient, float nyquist, uint32_t channel);

  /*!
   * @brief Resets the estimator by clearing the vectors storing the thdn, magnitude and frequency.
   *
   * @param [in] channel Channel which will be reset.
   * @return IasResult Error code. Returns eIasOk, if no error occured.
   * @retval eIasInitFailed       Thdn estimator has not yet been initialized.
   * @retval eIasInvalidParameter The given channel exeeds the configured maximum number of channels.
   * @retval eIasOk               Reset succeeded.
   */
  IasResult reset(uint32_t channel);

  /*!
   * @brief Returns the vector storing the magnitude values of each previously processed frame.
   *
   * @param [in] channel Channel whos data will be returned.
   * @return Vector of 'float' storing all magnitude values.
   */
  const std::vector<float>& getMagnitude(uint32_t channel) { return mMagnitude[channel]; };

  /*!
   * @brief Returns the vector storing the THDN values of each previously processed frame.
   *
   * @param [in] channel Channel whos data will be returned.
   * @return Vector of 'float' storing all THDN values.
   */
  const std::vector<float>& getThdn(uint32_t channel) { return mThdn[channel]; };

  /*!
   * @brief Returns the vector storing the frequency values of each previously processed frame.
   *
   * @param [in] channel Channel whos data will be returned.
   * @return Vector of 'float' storing all frequency values.
   */
  const std::vector<float>& getFrequency(uint32_t channel) { return mFrequency[channel]; };

private:

  /*!
   * @brief describes the limits of magnitude and thdn intervalls as indexes
   */
  struct IasBounds
  {
    IasBounds() {};
    IasBounds(uint32_t _lower, uint32_t _higher)
    {
      lower = _lower;
      higher = _higher;
    }
    uint32_t lower;
    uint32_t higher;
  };

  /*!
    * @brief Copy constructor, private unimplemented to prevent misuse.
    */
  IasThdnEstimator(IasThdnEstimator const &other); //lint !e1704

  /*!
    * @brief Assignment operator, private unimplemented to prevent misuse.
    */
  IasThdnEstimator& operator=(IasThdnEstimator const &other); //lint !e1704

  /*!
   * @brief Calculate the window
   *
   * @param [in] windowType Type of the window that should be applied on the signal
   *                        buffer, which is then transformed into frequency domain.
   */
  void generateWindow();

  /*!
   * @brief Generates the bounds for magnitude and thdn calculation
   *
   * @param [in] centerFreq Current sweep frequency
   * @param [in] channel    Channel for which bounds will be generated.
   * @return     Bounds struct containing the indizes at which
   *             the magnitude interval starts and ends.
   */
  IasBounds generateBounds(float centerFreq, uint32_t channel);

  /*!
   * @brief Calculate thdn for current frame
   *
   * @param [in] bounds  Define the intervals of thdn and magnitude values.
   * @param [in] channel Channel for which the thdn is calculated.
   */
  void calcThdn(IasBounds bounds, uint32_t channel);

  /*!
   * @brief Calculate magnitude for current frame
   *
   * @param [in] bounds  Define the intervals of thdn and magnitude values.
   * @param [in] channel Channel for which the magnitude is calculated.
   */
  void calcMag(IasBounds bounds, uint32_t channel);

  /*!
   * @brief Push new samples into the linear buffer and remove the same number from the start
   *
   * @param [in] input       Pointer to the beginning of the input signal. The input signal may have
   *                         one of the following types: 'float'. 'int16_t' or 'int32_t'.
   * @param [in] newSamples  Number of new samples to be added to the end / removed from the beginning
   *                         of the internal linear buffer
   * @param [in] channel     Channel to which data will be pushed.
   * @param [in] inputStride Offset between two samples in the input array. Default is '1' for deinterleaved
   *                         data.
   */
  template <typename T>
  void pushBackSamples(T *input, uint32_t newSamples, uint32_t channel, uint32_t inputStride = 1);

  IasFftFunctions *mFftFunction;   //<! FFT function

  bool      mIsInitialized;   //<! True if initialized
  uint32_t    mNumChannels;     //<! Number of processable channels
  uint32_t    mFftLength;       //<! Length of the window for the FFT
  uint32_t    mLdFftLength;     //<! log2 of mFftLength

  float   mWindowNorm;      //<! Factor to normalize the window's energy
  float  *mSignal;          //<! Array implementing a linear buffer for the signal
  float  *mWindow;          //<! Array with the windows values
  float  *mWorkingBuffer;   //<! Array for storing windowed signal and abs spectrum
  float  *mPds;             //<! Power density spectrum

  uint32_t   *mCurrentSample;   //<! Current sample for calculating the frequency
  float  *mSweepGradient;   //<! Gradient of the sweep [Hz / Sample]
  float  *mSweepStart;      //<! Starting frequency of the sweep [Hz]
  float   mNyquist;         //<! Nyquist frequency of the estimator [Hz]

  std::vector<std::vector<float>> mMagnitude;      //<! Stores the resulting magnitudes for each frame
  std::vector<std::vector<float>> mThdn;           //<! Stores the resulting THDN for each frame
  std::vector<std::vector<float>> mFrequency;      //<! Stores the frequencies of each corresponding frame
};


/**
 * @brief Function to get a IasThdnEstimator::IasResult as string.
 *
 * @return String carrying the result message.
 */
__attribute__ ((visibility ("default"))) std::string toString(const IasThdnEstimator::IasResult& type);


} //namespace IasAudio

#endif /* IAS_THDN_ESTIMATOR_HPP_ */
