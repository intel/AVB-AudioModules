/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasIntProcMutex.hpp
 * @date   2016
 * @brief
 */

#ifndef IASINTPROCMUTEX_HPP_
#define IASINTPROCMUTEX_HPP_

#include <atomic>

#include "audio/common/IasAudioCommonTypes.hpp"


namespace IasAudio {

/**
 * @brief This class provides the functionality of a process shared mutex
 */
class __attribute__ ((visibility ("default"))) IasIntProcMutex
{
  public:
    /**
     * @brief The result type for the IasIntProcMutex methods
     */
    enum IasResult
    {
      eIasOk,                         //!< Operation successful
      eIasNotInitialized,             //!< Not initialized
      eIasMutexAttrInitFailed,        //!< pthread_mutexattr_init failed
      eIasMutexAttrSetpsharedFailed,  //!< pthread_mutexattr_setpshared failed
      eIasMutexAttrSetrobustFailed,   //!< pthread_mutexattr_setrobust failed
      eIasMutexAttrSetprotocolFailed, //!< pthread_mutexattr_setprotocol failed
      eIasMutexInitFailed,            //!< pthread_mutex_init failed
      eIasMutexLockFailed,            //!< pthread_mutex_lock failed
      eIasMutexUnlockFailed,          //!< pthread_mutex_unlock failed
    };

    /**
     * @brief Constructor
     */
    IasIntProcMutex();

    /**
     * @brief Destructor
     */
    virtual ~IasIntProcMutex();

    /**
     * @brief Lock the mutex
     *
     * @returns The status of this call or the error that occurred during initialization
     * @retval eIasOk Mutex was locked
     * @retval eIasNotInitialized Not initialized
     * @retval eIasMutexAttrInitFailed pthread_mutexattr_init failed
     * @retval eIasMutexAttrSetpsharedFailed pthread_mutexattr_setpshared failed
     * @retval eIasMutexAttrSetrobustFailed pthread_mutexattr_setrobust failed
     * @retval eIasMutexAttrSetprotocolFailed pthread_mutexattr_setprotocol failed
     * @retval eIasMutexInitFailed pthread_cond_init failed
     * @retval eIasMutexLockFailed pthread_mutex_lock failed
     */
    IasResult lock();

    /**
     * @brief Try to lock the mutex
     *
     * Return immediately without waiting for the mutex to be available.
     *
     * @returns The status of this call or the error that occurred during initialization
     * @retval eIasOk Mutex was locked
     * @retval eIasNotInitialized Not initialized
     * @retval eIasMutexAttrInitFailed pthread_mutexattr_init failed
     * @retval eIasMutexAttrSetpsharedFailed pthread_mutexattr_setpshared failed
     * @retval eIasMutexAttrSetrobustFailed pthread_mutexattr_setrobust failed
     * @retval eIasMutexAttrSetprotocolFailed pthread_mutexattr_setprotocol failed
     * @retval eIasMutexInitFailed pthread_cond_init failed
     * @retval eIasMutexLockFailed pthread_mutex_lock failed
     */
    IasResult trylock();

    /**
     * @brief Unlock the mutex
     *
     * @returns The status of this call or the error that occurred during initialization
     * @retval eIasOk Mutex was unlocked
     * @retval eIasNotInitialized Not initialized
     * @retval eIasMutexAttrInitFailed pthread_mutexattr_init failed
     * @retval eIasMutexAttrSetpsharedFailed pthread_mutexattr_setpshared failed
     * @retval eIasMutexAttrSetrobustFailed pthread_mutexattr_setrobust failed
     * @retval eIasMutexAttrSetprotocolFailed pthread_mutexattr_setprotocol failed
     * @retval eIasMutexInitFailed pthread_cond_init failed
     * @retval eIasMutexUnlockFailed pthread_mutex_unlock failed
     */
    IasResult unlock();

    /**
     * @brief Get the native handle of the mutex
     *
     * @returns The native handle of the mutex
     */
    inline pthread_mutex_t* nativeHandle() { return &mMutex; }

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
    IasIntProcMutex(IasIntProcMutex const &other);

    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasIntProcMutex& operator=(IasIntProcMutex const &other);

    IasResult               mInitResult;
    pthread_mutex_t         mMutex;
    std::atomic<int32_t> mNativeResult;
};

/**
 * @brief Function to get a IasIntProcMutex::IasResult as string.
 * @return Enum Member as string
 */
std::string toString(const IasIntProcMutex::IasResult& type);


/**
 * @brief Lock guard according to RAII idiom for the IasIntProcMutex
 */
class __attribute__ ((visibility ("default"))) IasLockGuard
{
  public:
    /**
     * @brief Constructor
     *
     * Locks the mutex given via parameter
     *
     * @param[in] mutex
     */
    explicit IasLockGuard(IasIntProcMutex *mutex)
      :mMutex(mutex)
    {
      IasIntProcMutex::IasResult mutres = mMutex->lock();
      IAS_ASSERT(mutres == IasIntProcMutex::eIasOk);
      (void)mutres;
    }

    /**
     * @brief Destructor
     *
     * Unlocks the mutex given to constructor as parameter
     */
    ~IasLockGuard()
    {
      IasIntProcMutex::IasResult mutres = mMutex->unlock();
      IAS_ASSERT(mutres == IasIntProcMutex::eIasOk);
      (void)mutres;
    }

    /**
     * @brief Delete copy constructor
     */
    IasLockGuard(IasLockGuard const &) = delete;
    /**
     * @brief Delete assignment operator
     */
    IasLockGuard& operator=(IasLockGuard const &) = delete;

  private:
    IasIntProcMutex   *mMutex;
};

} /* namespace IasAudio */

#endif /* IASINTPROCMUTEX_HPP_ */
