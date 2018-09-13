/*
 * @COPYRIGHT_TAG@
 */
/**
 * @file    IasFilterEstimator.hpp
 * @brief   Estimation of unknown filter impulse responses based on the NLMS algorithm.
 * @date    2016
 */

#ifndef IAS_FILTER_ESTIMATOR_HPP_
#define IAS_FILTER_ESTIMATOR_HPP_

#include <sndfile.h>
#include <string>
#include <vector>



namespace IasAudio {

/*!
 * @brief Documentation for class IasFilterEstimator.
 *
 * This class implements an estimator for unknown filter impulse responses based on the NLMS algorithm.
 */
class __attribute__ ((visibility ("default"))) IasFilterEstimator
{
public:
  /*!
   * @brief Return type of most class methods.
   */
  enum IasResult
  {
    eIasOk = 0,                   //!< Success
    eIasInvalidParameter,         //!< Invalid parameter
    eIasOutOfMemory,              //!< Out of memory
    eIasInitFailed,               //!< Program is not initialized
    eIasAlreadyInitialized,       //!< Already initialized
    eIasCorruptedFile,            //!< File already closed or currupted
    eIasIdentificationIncommplete //!< System has not been identified (system distance is worse than specified threshold)
  };

  IasFilterEstimator();
  ~IasFilterEstimator();

  /*!
   * @brief Prepare the filter estimator for operation.
   *
   * @param [in] input           .wav file with clean input signal.
   * @param [in] output          .wav file with filtered signal.
   * @param [in] errorSignal     .wav file to write the residual error to.
   * @param [in] impulseResponse .wav file to write the estimated IR to.
   * @param [in] filterLength    filter length of the estimated filter.
   * @return Result enum variable giving information about how successful the function call was.
   * @retval eIasOk                 Function call was successful.
   * @retval eIasInvalidParameter   At least one input parameter was invald.
   * @retval eIasOutOfMemory        Couldn't allocate enough memory.
   * @retval eIasAlreadyInitialized The filter estimator is already initialized.
   */
  IasResult startUp(const std::string& input,
                    const std::string& output,
                    const std::string& errorSignal,
                    const std::string& impulseResponse,
                    uint32_t filterLength);

  /*!
   * @brief Shut down the filter estimator and free memory.
   *
   * @return Result enum variable giving information about how successful the function call was.
   * @retval eIasOk            Function call was successful.
   * @retval eIasCorruptedFile An audio file couldn't be closed.
   */
  IasResult shutDown();

  /*!
   * @brief Performs a least mean square algorithm to estimate the filter.
   *
   * @param [in] threshold  Threshold in dB for the residual error. If the residual error
   *                        deceeds the threshold, the program stops.
   * @param [out] estimated True if the filter has been estimated and the residual error
   *                        is beneath the given threshold. False otherwise.
   *
   * @return Result enum variable giving information whether the function call was successful.
   * @retval eIasOk                        System identification has been completed.
   * @retval eIasInitFailed                Filter estimator has to be initialized first.
   * @retval eIasInvalidParameter          Threshold exceeds limits.
   * @retval eIasIdentificationIncommplete System identification has not been completed (system distance above threshold).
   */
  IasResult lms(float threshold);

private:
  /*!
   * @brief Updates the internal state and the IR.
   *
   * @param [in] xHat    Output sample from estimated filter.
   * @param [in] xTilde  Output sample from real filter.
   * @param [in] channel Current channel operated on.
   * @return Mean of squared errors.
   */
  float updateSystem(float xHat, float xTilde, uint32_t channel);

  /*!
   * @brief Filters the input signal with the estimated filter.
   *
   * @param [in] channel Current channel operated on.
   * @return Fitler result.
   */
  float filterInput(uint32_t channel);

  bool mIsInitialized;     //!< Result variable set to true after initialization.

  uint32_t   mFilterLength;  //!< Filter length of the estimated filter.
  uint32_t   mNumChannels;   //!< Number of channels.
  float *mFilter;        //!< Array storing all filter coefficients.

  SNDFILE *mInputFile;  //!< Pointer to input file.
  SNDFILE *mOutputFile; //!< Pointer to output file.
  SNDFILE *mErrorFile;  //!< Pointer to error file.
  SNDFILE *mIRFile;     //!< Pointer to IR file.

  float *mInputBuffer;   //!< Ring buffer storing the input signal.
  float *mOutputSample;  //!< Array storing last output sample for each channel.
  uint32_t   mInputWrite;    //!< Input ring buffers current write position.

  float *mSysDist; //!< Stores the last 32 residual errors for averaging purposes.
  float  mAlpha;   //!< Stepwidth of the LMS algorithm. 1.0 in this case, since no noise is applied.
};


/**
 * @brief Function to get a IasFilterEstimator::IasResult as string.
 *
 * @return String carrying the result message.
 */
__attribute__ ((visibility ("default"))) std::string toString(const IasFilterEstimator::IasResult& type);


} // namespace IasAudio

#endif // IAS_FILTER_ESTIMATOR_HPP_
