/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasIntProcCondVar.hpp
 * @date   2016
 * @brief
 */

#ifndef IASINTPROCCONDVAR_HPP_
#define IASINTPROCCONDVAR_HPP_

#include <atomic>

#include "audio/common/IasAudioCommonTypes.hpp"

namespace IasAudio {

class IasIntProcMutex;

/**
 * @brief This class provides the functionality of a process shared condition variable
 */
class __attribute__ ((visibility ("default"))) IasIntProcCondVar
{
  public:
    /**
     * @brief The result type for the IasIntProcCondVar methods
     */
    enum IasResult
    {
      eIasOk,                         //!< Operation successful
      eIasInvalidParam,               //!< Invalid parameter, e.g. mutex==nullptr
      eIasNotInitialized,             //!< Not initialized
      eIasCondAttrInitFailed,         //!< pthread_condattr_init failed
      eIasCondAttrSetpsharedFailed,   //!< pthread_condattr_setpshared failed
      eIasCondAttrSetclockFailed,     //!< pthread_condattr_setclock failed
      eIasCondInitFailed,             //!< pthread_cond_init failed
      eIasTimeout,                    //!< Timeout while waiting for condition variable
      eIasCondWaitFailed,             //!< pthread_cond_wait failed
      eIasClockTimeNotAvailable,      //!< Clock time not available
      eIasCondSignalFailed,           //!< pthread_cond_signal failed
    };

    /**
     * @brief Constructor
     */
    IasIntProcCondVar();

    /**
     * @brief Destructor
     */
    virtual ~IasIntProcCondVar();

    /**
     * @brief Wait for the condition variable to be signaled
     *
     * @param[in] mutex The mutex to be used for the condition variable
     *
     * @returns The status of this call or the error that occurred during initialization
     * @retval eIasOk Condition variable was signaled
     * @retval eIasInvalidParam Mutex not initialized
     * @retval eIasNotInitialized Not initialized
     * @retval eIasCondAttrInitFailed pthread_condattr_init failed
     * @retval eIasCondAttrSetpsharedFailed pthread_condattr_setpshared failed
     * @retval eIasCondAttrSetclockFailed pthread_condattr_setclock failed
     * @retval eIasCondInitFailed pthread_cond_init failed
     * @retval eIasCondWaitFailed pthread_cond_wait failed
     */
    IasResult wait(IasIntProcMutex &mutex);

    /**
     * @brief Wait for the condition variable to be signaled for a certain time
     *
     * @param[in] mutex The mutex to be used for the condition variable
     * @param[in] time_ms timespan to wait at maximum for a condition variable
     *
     * @returns The status of the method call
     * @retval eIasOk Condition variable was signaled before timeout
     * @retval eIasInvalidParam Mutex not initialized
     * @retval eIasNotInitialized Not initialized
     * @retval eIasCondAttrInitFailed pthread_condattr_init failed
     * @retval eIasCondAttrSetpsharedFailed pthread_condattr_setpshared failed
     * @retval eIasCondAttrSetclockFailed pthread_condattr_setclock failed
     * @retval eIasCondInitFailed pthread_cond_init failed
     * @retval eIasTimeout Timeout while waiting for condition variable
     * @retval eIasCondWaitFailed pthread_cond_wait failed
     * @retval eIasClockTimeNotAvailable Clock time not available
     */
    IasResult wait(IasIntProcMutex &mutex, uint64_t time_ms);

    /**
     * @brief Signal the condition variable
     *
     * @returns The status of this call or the error that occurred during initialization
     * @retval eIasOk Condition variable was signaled
     * @retval eIasInvalidParam Mutex not initialized
     * @retval eIasNotInitialized Not initialized
     * @retval eIasCondAttrInitFailed pthread_condattr_init failed
     * @retval eIasCondAttrSetpsharedFailed pthread_condattr_setpshared failed
     * @retval eIasCondAttrSetclockFailed pthread_condattr_setclock failed
     * @retval eIasCondInitFailed pthread_cond_init failed
     * @retval eIasCondSignalFailed pthread_cond_signal failed
     */
    IasResult signal();

    /**
     * @brief Return the native result of the pthread function calls
     *
     * This can be used for debugging purposes
     *
     * @returns The native error code from all pthread function calls
     */
    inline int32_t nativeResult() const { return mNativeResult; }

  private:
    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasIntProcCondVar(IasIntProcCondVar const &other);

    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasIntProcCondVar& operator=(IasIntProcCondVar const &other);

    IasResult               mInitResult;
    pthread_cond_t          mCondVar;
    std::atomic<int32_t>    mNativeResult;
};

/**
 * @brief Function to get a IasIntProcCondVar::IasResult as string.
 * @return Enum Member as string
 */
std::string toString(const IasIntProcCondVar::IasResult& type);


} /* namespace IasAudio */

#endif /* IASINTPROCCONDVAR_HPP_ */
