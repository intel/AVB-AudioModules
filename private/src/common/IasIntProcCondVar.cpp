/*
 * @COPYRIGHT_TAG@
 */
/**
 * @file   IasIntProcCondVar.cpp
 * @date   2016
 * @brief
 */

#include <pthread.h>
#include "internal/audio/common/IasIntProcCondVar.hpp"
#include "internal/audio/common/IasIntProcMutex.hpp"
#include <chrono>
#include <iostream>

namespace IasAudio {

IasIntProcCondVar::IasIntProcCondVar()
  :mInitResult(eIasNotInitialized)
  ,mNativeResult(0)
{
  pthread_condattr_t condAttr;
  bool condAttrInitialized = false;
  int32_t nativeResult = pthread_condattr_init(&condAttr);
  if (nativeResult == 0)
  {
    condAttrInitialized = true;
    nativeResult = pthread_condattr_setpshared(&condAttr, PTHREAD_PROCESS_SHARED);
    if (nativeResult == 0)
    {
      nativeResult = pthread_condattr_setclock(&condAttr, CLOCK_MONOTONIC);
      if (nativeResult == 0)
      {
        nativeResult = pthread_cond_init(&mCondVar, &condAttr);
        if (nativeResult == 0)
        {
          mInitResult = eIasOk;
        }
        else
        {
          mInitResult = eIasCondInitFailed;
        }
      }
      else
      {
        mInitResult = eIasCondAttrSetclockFailed;
      }
    }
    else
    {
      mInitResult = eIasCondAttrSetpsharedFailed;
    }
  }
  else
  {
    mInitResult = eIasCondAttrInitFailed;
  }

  mNativeResult = nativeResult;

  if (condAttrInitialized == true)
  {
    pthread_condattr_destroy(&condAttr);
  }
}

IasIntProcCondVar::~IasIntProcCondVar()
{
  if (mInitResult == eIasOk)
  {
    // Workaround: Reinitialization of condvar is done because the condvar might become invalid due to a dead client.
    // In that case pthread_cond_destroy() won't return and block forever!
    pthread_condattr_t condAttr;
    int32_t result = pthread_condattr_init(&condAttr);
    result = pthread_cond_init(&mCondVar, &condAttr);
    result = pthread_cond_destroy(&mCondVar);
    result = pthread_condattr_destroy(&condAttr);
    // We don't care about the result. Just to make Klocwork happy.
    (void) result;
  }
  mInitResult = eIasNotInitialized;
}

IasIntProcCondVar::IasResult IasIntProcCondVar::wait(IasIntProcMutex &mutex)
{
  if (mInitResult != eIasOk)
  {
    return mInitResult;
  }
  const int32_t nativeResult = pthread_cond_wait(&mCondVar, mutex.nativeHandle());
  mNativeResult = nativeResult;
  if (nativeResult != 0)
  {
    return eIasCondWaitFailed;
  }
  return eIasOk;
}

IasIntProcCondVar::IasResult IasIntProcCondVar::wait(IasIntProcMutex &mutex, uint64_t time_ms)
{
  if (mInitResult != eIasOk)
  {
    return mInitResult;
  }

  std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
  std::chrono::steady_clock::time_point then = now + std::chrono::milliseconds(time_ms);

  auto sec =std::chrono::time_point_cast<std::chrono::seconds>(then);
  auto nsec =std::chrono::time_point_cast<std::chrono::nanoseconds>(then);

  struct timespec timespecValue;
  timespecValue.tv_sec = sec.time_since_epoch().count();
  timespecValue.tv_nsec = (nsec.time_since_epoch().count() %1000000000ULL);

  const int32_t nativeResult = pthread_cond_timedwait(&mCondVar, mutex.nativeHandle(), &timespecValue);

  mNativeResult = nativeResult;
  if (nativeResult == 0)
  {
    return eIasOk;
  }
  else if (nativeResult == ETIMEDOUT)
  {
    return eIasTimeout;
  }
  else
  {
    return eIasCondWaitFailed;
  }
}

IasIntProcCondVar::IasResult IasIntProcCondVar::signal()
{
  if (mInitResult != eIasOk)
  {
    return mInitResult;
  }
  const int32_t nativeResult = pthread_cond_signal(&mCondVar);
  mNativeResult = nativeResult;
  if (nativeResult != 0)
  {
    return eIasCondSignalFailed;
  }
  return eIasOk;
}

#define STRING_RETURN_CASE(name) case name: return std::string(#name); break
#define DEFAULT_STRING(name) default: return std::string(name)

__attribute__ ((visibility ("default"))) std::string toString(const IasIntProcCondVar::IasResult& type)
{
  switch(type)
  {
    STRING_RETURN_CASE(IasIntProcCondVar::eIasOk);
    STRING_RETURN_CASE(IasIntProcCondVar::eIasInvalidParam);
    STRING_RETURN_CASE(IasIntProcCondVar::eIasNotInitialized);
    STRING_RETURN_CASE(IasIntProcCondVar::eIasCondAttrInitFailed);
    STRING_RETURN_CASE(IasIntProcCondVar::eIasCondAttrSetpsharedFailed);
    STRING_RETURN_CASE(IasIntProcCondVar::eIasCondAttrSetclockFailed);
    STRING_RETURN_CASE(IasIntProcCondVar::eIasCondInitFailed);
    STRING_RETURN_CASE(IasIntProcCondVar::eIasTimeout);
    STRING_RETURN_CASE(IasIntProcCondVar::eIasCondWaitFailed);
    STRING_RETURN_CASE(IasIntProcCondVar::eIasClockTimeNotAvailable);
    STRING_RETURN_CASE(IasIntProcCondVar::eIasCondSignalFailed);
    DEFAULT_STRING("Invalid IasIntProcCondVar::IasResult => " + std::to_string(type));
  }
}


} /* namespace IasAudio */
