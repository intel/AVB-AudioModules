/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file    IasWavComp.hpp
 * @brief   Compare two audio (WAVE) files, by calculating the difference.
 * @date    2016
 */

#ifndef IASWAVCOMP_HPP_
#define IASWAVCOMP_HPP_

#include <sndfile.h>
#include <string>



namespace IasAudio {


/**
 * @brief Read and compare audio files.
 */
class __attribute__ ((visibility ("default"))) IasWavComp
{
public:
  IasWavComp();
  ~IasWavComp();

  /*!
   * @brief The return values of the methods.
   */
  enum IasResult
  {
    eIasOk,             //<! Success
    eIasMemoryError,    //<! Couldn't allocate required space
    eIasInvalidParam    //<! Input parameter exceeds threshold
  };

  /**
   * @brief Stores flags to store differences between two audio files
   */
  enum IasDiffs
  {
    eIasNoDiffs = 0,            //<! Files don't differ in any way
    eIasDiffRate = 1 << 0,      //<! Samplerate differs
    eIasDiffChannels = 1 << 1,  //<! Channel number differs
    eIasDiffFormat = 1 << 2,    //<! Audio format differs
    eIasDiffLength = 1 << 3,    //<! Signal length in samples differs
    eIasDiffEnergy = 1 << 4     //<! Data deviation exceeds threshold
  };

  /**
   * @brief Reads and compares two audio files, by calculating the difference.
   *
   * Uses the snd audio lib to read two audio signals.
   * Then compares the signals for every aspect given in the 'IasDiffs' flags.
   *
   * @param [in] file1 Path to first audio file.
   * @param [in] file2 Path to second audio file.
   *
   * @return Program status.
   * @retval eIasOk             Everything worked finde.
   * @retval eIasMemoryError    Couldn't allocate required space.
   * @retval eIasInvalidParam   Input parameter exceeds threshold.
   */
  IasResult compareFiles(const std::string& file1, const std::string& file2);

  /**
   * @brief Set the threshold for acceptable magnitude deviation.
   *
   * @param [in] threshold Threshold in dB. Must be smaller than +0dB.
   *
   * @return Program status.
   * @retval eIasOk             Everything worked finde.
   * @retval eIasInvalidParam   Threshold is larger than +0dB.
   */
  IasResult setEnergyThreshold(float threshold);

  /**
   * @brief Set a conditions which trigger a fail in the evaluation function
   *
   * If you want the evaluate() function to only return 'false' for a certain flag,
   * then you have to specify it in this function. You can also turn off certain flags.
   *
   * @param [in] diff Condition which will be triggered.
   * @param [in] on   True, if you want to turn it on, false otherwise.
   */
  void setCondition(IasDiffs diff, bool on);

  /**
   * @brief Evaluates comparision data after call to 'compareFiles' function.
   *
   * 'compareFiles' gatheres differences between two files. This function
   * prints out all encountered differences and returns true or false,
   * depending on the set and gathered flags.
   *
   * @return True or false depending on differences and set flags.
   */
  bool evaluate();

private:

  /*!
   *  @brief Copy constructor, private unimplemented to prevent misuse.
   */
  IasWavComp(IasWavComp const &other);

  /*!
   *  @brief Assignment operator, private unimplemented to prevent misuse.
   */
  IasWavComp& operator=(IasWavComp const &other);

  /**
   * @brief Resets member values and storage for certain number of channels.
   *
   * @return Program status.
   * @retval eIasOk             Everything worked fine.
   * @retval eIasMemoryError    Couldn't allocate required space.
   */
  IasResult reset();

  /**
   * @brief Compares file infos and sets flags if a difference is found.
   */
  void compareFileInfos();

  SNDFILE *mFile1;      //<! First input file
  SNDFILE *mFile2;      //<! Second input file

  SF_INFO mFileInfo1;   //<! Info of first input file
  SF_INFO mFileInfo2;   //<! info of second input file

  float *mDataBuffer1;   //<! Stores mFrameLength * mNumChannels samples of first input
  float *mDataBuffer2;   //<! Stores mFrameLength * mNumChannels samples of second input

  const uint32_t mFrameLength = 1024;        //<! Frame length
  float mThresholdEnergy;        //<! Threshold for energy difference flag to trigger

  uint32_t  mNumChannels;    //<! Current number of channels
  float *mDiffEnergy;    //<! Current energy difference for each channel

  uint32_t mLength1;         //<! Length of first input file
  uint32_t mLength2;         //<! Length of second input file
  uint32_t mDiffFlags;       //<! Set of flags describing the difference between two files
  uint32_t mBoolMask;        //<! Controls whether a certain flag results in a failure.
};


/**
 * @brief Function to get a IasWavComp::IasResult as string.
 *
 * @return String carrying the result message.
 */
__attribute__ ((visibility ("default"))) std::string toString(const IasWavComp::IasResult& type);


} // namespace IasAudio

#endif // IASWAVCOMP_HPP_
