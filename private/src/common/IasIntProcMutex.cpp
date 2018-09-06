/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasIntProcMutex.cpp
 * @date   2016
 * @brief
 */

#include <pthread.h>
#include "internal/audio/common/IasIntProcMutex.hpp"

namespace IasAudio {

IasIntProcMutex::IasIntProcMutex()
  :mInitResult(eIasNotInitialized)
  ,mNativeResult(0)
{
  pthread_mutexattr_t mutexAttr;
  bool mutexAttrInitialized = false;
  int32_t nativeResult = pthread_mutexattr_init(&mutexAttr);
  if (nativeResult != 0)
  {
    mInitResult = eIasMutexAttrInitFailed;
  }
  else
  {
    mutexAttrInitialized = true;
    nativeResult = pthread_mutexattr_setpshared(&mutexAttr, PTHREAD_PROCESS_SHARED);
  }

  if (nativeResult != 0)
  {
    mInitResult = eIasMutexAttrSetpsharedFailed;
  }
  else
  {
    nativeResult = pthread_mutexattr_setrobust(&mutexAttr, PTHREAD_MUTEX_ROBUST);
  }

  if (nativeResult != 0)
  {
    mInitResult = eIasMutexAttrSetrobustFailed;
  }
  else
  {
    nativeResult = pthread_mutexattr_setprotocol(&mutexAttr, PTHREAD_PRIO_INHERIT);
  }

  if (nativeResult != 0)
  {
    mInitResult = eIasMutexAttrSetprotocolFailed;
  }
  else
  {
    nativeResult = pthread_mutex_init(&mMutex, &mutexAttr);
  }

  if (nativeResult != 0)
  {
    mInitResult = eIasMutexInitFailed;
  }
  else
  {
    mInitResult = eIasOk;
  }

  mNativeResult = nativeResult;

  if (mutexAttrInitialized == true)
  {
    pthread_mutexattr_destroy(&mutexAttr);
  }
}

IasIntProcMutex::~IasIntProcMutex()
{
  if (mInitResult == eIasOk)
  {
    int32_t result = pthread_mutex_destroy(&mMutex);
    // We don't care about the result. Just to make Klocwork happy.
    (void)result;
  }
  mInitResult = eIasNotInitialized;
}

IasIntProcMutex::IasResult IasIntProcMutex::lock()
{
  if (mInitResult != eIasOk)
  {
    return mInitResult;
  }
  int32_t nativeResult = pthread_mutex_lock(&mMutex);
  mNativeResult = nativeResult;
  if (nativeResult == 0)
  {
    return eIasOk;
  }
  else if (nativeResult == EOWNERDEAD)
  {
    nativeResult = pthread_mutex_consistent(&mMutex);
    mNativeResult = nativeResult;
    if (nativeResult == 0)
    {
      nativeResult = pthread_mutex_unlock(&mMutex);
      mNativeResult = nativeResult;
    }
    else
    {
      return eIasMutexLockFailed;
    }

    if (nativeResult == 0)
    {
      nativeResult = pthread_mutex_lock(&mMutex);
      mNativeResult = nativeResult;
    }
    else
    {
      return eIasMutexLockFailed;
    }
  }
  else
  {
    return eIasMutexLockFailed;
  }
  return eIasOk;
}

IasIntProcMutex::IasResult IasIntProcMutex::trylock()
{
  if (mInitResult != eIasOk)
  {
    return mInitResult;
  }
  int32_t nativeResult = pthread_mutex_trylock(&mMutex);
  mNativeResult = nativeResult;
  if (nativeResult == 0)
  {
    return eIasOk;
  }
  else if (nativeResult == EOWNERDEAD)
  {
    nativeResult = pthread_mutex_consistent(&mMutex);
    mNativeResult = nativeResult;
    if (nativeResult == 0)
    {
      nativeResult = pthread_mutex_unlock(&mMutex);
      mNativeResult = nativeResult;
    }
    else
    {
      return eIasMutexLockFailed;
    }

    if (nativeResult == 0)
    {
      nativeResult = pthread_mutex_trylock(&mMutex);
      mNativeResult = nativeResult;
    }
    else
    {
      return eIasMutexLockFailed;
    }
  }
  else
  {
    return eIasMutexLockFailed;
  }
  return eIasOk;
}

IasIntProcMutex::IasResult IasIntProcMutex::unlock()
{
  if (mInitResult != eIasOk)
  {
    return mInitResult;
  }
  const int32_t nativeResult = pthread_mutex_unlock(&mMutex);
  mNativeResult = nativeResult;
  if (nativeResult != 0)
  {
    return eIasMutexUnlockFailed;
  }
  return eIasOk;
}


#define STRING_RETURN_CASE(name) case name: return std::string(#name); break
#define DEFAULT_STRING(name) default: return std::string(name)

__attribute__ ((visibility ("default"))) std::string toString(const IasIntProcMutex::IasResult& type)
{
  switch(type)
  {
    STRING_RETURN_CASE(IasIntProcMutex::eIasOk);
    STRING_RETURN_CASE(IasIntProcMutex::eIasNotInitialized);
    STRING_RETURN_CASE(IasIntProcMutex::eIasMutexAttrInitFailed);
    STRING_RETURN_CASE(IasIntProcMutex::eIasMutexAttrSetpsharedFailed);
    STRING_RETURN_CASE(IasIntProcMutex::eIasMutexAttrSetrobustFailed);
    STRING_RETURN_CASE(IasIntProcMutex::eIasMutexAttrSetprotocolFailed);
    STRING_RETURN_CASE(IasIntProcMutex::eIasMutexInitFailed);
    STRING_RETURN_CASE(IasIntProcMutex::eIasMutexLockFailed);
    STRING_RETURN_CASE(IasIntProcMutex::eIasMutexUnlockFailed);
    DEFAULT_STRING("Invalid IasIntProcMutex::IasResult => " + std::to_string(type));
  }
}



} /* namespace IasAudio */
