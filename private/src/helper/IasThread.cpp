/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/
/**
 * @file
 */

#include "internal/audio/common/helper/IasThread.hpp"


#include <sys/prctl.h> // For setting/getting the thread's name.
#include <string.h>    // For strncpy copying the thread's name.
#include <sys/errno.h>

#define THREAD_NAME_LEN 16 // Corresponds to TASK_COMM_LEN in linux/sched.h  --> maximum size of thread name.
#define STRING_RETURN_CASE(name) case name: return std::string(#name); break
#define DEFAULT_STRING(name) default: return std::string(name)



template<>
int32_t logToDlt(DltContextData &log, IasAudio::IasThreadResult const & value)
{
  return logToDlt(log, toString(value));
}

template<>
int32_t logToDlt(DltContextData & log, IasAudio::IasThread::IasThreadSchedulingPolicy const & value)
{
  int32_t result;
  switch (value)
  {
    case IasAudio::IasThread::eIasSchedulingPolicyOther:
      result = logToDlt(log, "other");
      break;
    case IasAudio::IasThread::eIasSchedulingPolicyFifo:
      result = logToDlt(log, "fifo");
      break;
    case IasAudio::IasThread::eIasSchedulingPolicyRR:
      result = logToDlt(log, "rr");
      break;
    case IasAudio::IasThread::eIasSchedulingPolicyBatch:
      result = logToDlt(log, "batch");
      break;
    case IasAudio::IasThread::eIasSchedulingPolicyIdle:
      result = logToDlt(log, "idle");
      break;
    default:
      result = logToDlt(log, static_cast<int32_t>(value));
      break;
  }
  return result;
}


namespace IasAudio {

IasThreadId const cIasThreadIdInvalid = -1;

std::string toString(const IasAudio::IasThreadResult &res)
{
  switch (res)
  {
    STRING_RETURN_CASE(IasAudio::eIasThreadOk);
    STRING_RETURN_CASE(IasAudio::eIasThreadFailed);
    STRING_RETURN_CASE(IasAudio::eIasThreadAlreadyStarted);
    STRING_RETURN_CASE(IasAudio::eIasThreadNotRunning);
    STRING_RETURN_CASE(IasAudio::eIasCreateBarrierFailed);
    STRING_RETURN_CASE(IasAudio::eIasInitAttributeFailed);
    STRING_RETURN_CASE(IasAudio::eIasCreateThreadFailed);
    STRING_RETURN_CASE(IasAudio::eIasDestroyAttributeFailed);
    STRING_RETURN_CASE(IasAudio::eIasWaitBarrierFailed);
    STRING_RETURN_CASE(IasAudio::eIasJoinThreadFailed);
    STRING_RETURN_CASE(IasAudio::eIasThreadSetNameFailed);
    STRING_RETURN_CASE(IasAudio::eIasThreadGetNameFailed);
    STRING_RETURN_CASE(IasAudio::eIasThreadSchedulePriorityFailed);
    STRING_RETURN_CASE(IasAudio::eIasThreadSchedulePriorityNotPermitted);
    STRING_RETURN_CASE(IasAudio::eIasThreadSchedulingParameterInvalid);
    STRING_RETURN_CASE(IasAudio::eIasThreadSignalFailed);
    STRING_RETURN_CASE(IasAudio::eIasThreadObjectInvalid);
    DEFAULT_STRING("eIasThreadResultInvalid");
  }
}



IasThread::IasThread(IasIRunnable * runnableObject, std::string const &threadName, size_t stackSize)
  : mThreadName(threadName)
  , mStackSize(stackSize)
  , mAssureRunning(false)
  , mThreadState(cTHREAD_STATE_INVALID)
  , mThreadId(cIasThreadIdInvalid)
  , mRunnableObject(runnableObject)
  , mThreadStartedBarrier()
  , mThreadStartedBarrierInitialized(false)
  , mStartThreadResult(eIasThreadNotRunning)
  , mRunThreadResult(eIasThreadNotRunning)
  , mSchedulingPolicy(-1)
  , mSchedulingPriority(-1)
//  , mLog(IasAudioLogging::registerDltContext("THR", "Ias Thread"))
{

}

IasThread::~IasThread()
{

  if ( cTHREAD_STATE_INVALID != getCurrentThreadState() )
  {
    IasThreadResult const stopResult = stop();
    if (stopResult != eIasThreadOk)
    {
      if ( getCurrentThreadState()&cTHREAD_STATE_IS_STOPPING_FLAG )
      {
        //DLT_LOG_CXX(*mLog, DLT_LOG_WARN, "~IasThread failed to stop thread because it looks as if someone else is currently stopping it. Waiting for stop to finish to prevent form severe errors.", stopResult, getCurrentThreadState());
        // If the thread is currently stopped by someone else, wait for the stop to be finished.
        while ( getCurrentThreadState()&cTHREAD_STATE_IS_STOPPING_FLAG )
        {
          usleep(20000);
        }
        //DLT_LOG_CXX(*mLog, DLT_LOG_WARN, "~IasThread thread is now actually stopped. Continue destruction.", getCurrentThreadState());
      }
      else
      {
        //DLT_LOG_CXX(*mLog, DLT_LOG_WARN, "~IasThread failed to stop thread unexpectedly. State is", getCurrentThreadState());
      }
    }
  }

  if ( mThreadStartedBarrierInitialized )
  {
    // Ensure the barrier resources are freed.
    int32_t destroyResult = pthread_barrier_destroy(&(mThreadStartedBarrier));
    if(destroyResult != 0)
    {
      //DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, "~IasThread pthread_barrier_destroy failed");
    }
    mThreadStartedBarrierInitialized = false;
  }
}

IasThreadResult IasThread::setSchedulingParameters(IasThreadSchedulingPolicy policy, int32_t priority)
{
  mSchedulingPriority = priority;
  IasThreadResult result = processSchedulingParameters(policy, mSchedulingPriority, mSchedulingPolicy);
  if (isRunning())
  {
    IasThreadResult commitResult = commitSchedulingParameters(mThreadId, mSchedulingPolicy, mSchedulingPriority);
    if (result == eIasThreadOk)
    {
      result = commitResult;
    }
  }
  return result;
}

IasThreadResult IasThread::getSchedulingParameters(IasThreadSchedulingPolicy & policy, int32_t & priority)
{
  return getSchedulingParameters(mThreadId, policy, priority);
}

IasThreadResult IasThread::signal(const int32_t signum)
{
  return signal(mThreadId, signum);
}

IasThreadResult IasThread::setThreadName(const IasThreadId threadId, const std::string & name)
{
  IasThreadResult result = eIasThreadOk;
  std::string threadName = name;

  if (threadName.length() >= THREAD_NAME_LEN)
  {
    threadName.resize(THREAD_NAME_LEN-1);
   //DLT_LOG_CXX(*mLog, DLT_LOG_WARN, "setThreadName name will be truncated to 16 characters");
  }

  if (threadName.length() == 0)
  {
    //DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, "setThreadName is empty");
    result = eIasThreadSetNameFailed;
  }
  else
  {
    if (pthread_setname_np(threadId, threadName.c_str()) < 0)
    {
      //DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, "setThreadName failed");
      result = eIasThreadSetNameFailed;
    }
  }

  return result;
}

IasThreadResult IasThread::getThreadName(const IasThreadId threadId, std::string & name)
{
  char threadName[THREAD_NAME_LEN + 1];
  if (pthread_getname_np(threadId, threadName, THREAD_NAME_LEN) == 0)
  {
    name = threadName;
    return eIasThreadOk;
  }
  else
  {
    //DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, "getThreadName failed");
    return eIasThreadGetNameFailed;
  }
}

IasThreadResult IasThread::setSchedulingParameters(const IasThreadId threadId, IasThreadSchedulingPolicy policy, int32_t priority)
{
  int32_t schedulingPolicy;
  IasThreadResult result = processSchedulingParameters(policy, priority, schedulingPolicy);
  if (result == eIasThreadOk)
  {
    result = commitSchedulingParameters(threadId, schedulingPolicy, priority);
  }
  return result;
}

IasThreadResult IasThread::getSchedulingParameters(const IasThreadId threadId, IasThreadSchedulingPolicy & policy, int32_t & priority)
{
  IasThreadResult result = eIasThreadOk;
  int32_t localPolicy;
  sched_param schedParam;
  uint32_t pthreadResult = pthread_getschedparam(threadId, &localPolicy, &schedParam);
  if (pthreadResult == EPERM)
  {
    return eIasThreadSchedulePriorityNotPermitted;
  }
  else if (pthreadResult != 0)
  {
    return eIasThreadSchedulePriorityFailed;
  }
  if (result == eIasThreadOk)
  {
    switch (localPolicy)
    {
      case SCHED_OTHER:
        policy = eIasSchedulingPolicyOther;
        break;

      case SCHED_FIFO:
        policy = eIasSchedulingPolicyFifo;
        break;

      case SCHED_RR:
        policy = eIasSchedulingPolicyRR;
        break;

      case SCHED_BATCH:
        policy = eIasSchedulingPolicyBatch;
        break;

      case SCHED_IDLE:
        policy = eIasSchedulingPolicyIdle;
        break;
    }
    priority = schedParam.__sched_priority;
  }
  return result;
}

IasThreadResult IasThread::signal(const IasThreadId threadId, const int32_t signum)
{
  return pthread_kill(threadId, signum) == 0 ? eIasThreadOk : eIasThreadSignalFailed;
}

IasThreadResult IasThread::processSchedulingParameters(IasThreadSchedulingPolicy policy, int32_t & priority, int32_t & schedulingPolicy)
{
  IasThreadResult result = eIasThreadOk;

  switch (policy)
  {
    case eIasSchedulingPolicyOther:
      schedulingPolicy = SCHED_OTHER;
      break;

    case eIasSchedulingPolicyFifo:
      schedulingPolicy = SCHED_FIFO;
      break;

    case eIasSchedulingPolicyRR:
      schedulingPolicy = SCHED_RR;
      break;

    case eIasSchedulingPolicyBatch:
      schedulingPolicy = SCHED_BATCH;
      break;

    case eIasSchedulingPolicyIdle:
      schedulingPolicy = SCHED_IDLE;
      break;
  }

  // Check priority is in range.
  if (priority < sched_get_priority_min(schedulingPolicy))
  {
    priority = sched_get_priority_min(schedulingPolicy);
    result = eIasThreadSchedulingParameterInvalid;
  }
  else if (priority > sched_get_priority_max(schedulingPolicy))
  {
    priority = sched_get_priority_max(schedulingPolicy);
    result = eIasThreadSchedulingParameterInvalid;
  }

  return result;
}

IasThreadResult IasThread::commitSchedulingParameters(const IasThreadId threadId, const int32_t policy, const int32_t priority)
{
  IasThreadResult result = eIasThreadOk;
  if (policy != -1 && priority != -1)
  {
    sched_param schedParam;
    schedParam.__sched_priority = priority;
    int32_t res = pthread_setschedparam(threadId, policy, &schedParam);

    if (res == EPERM)
    {
      result = eIasThreadSchedulePriorityNotPermitted;
    }
    else if (res == EINVAL)
    {
      result = eIasThreadSchedulingParameterInvalid;
    }
    else if (res != 0)
    {
      result = eIasThreadSchedulePriorityFailed;
    }
  }

  return result;
}

IasThreadResult IasThread::start(bool assureRunning, IasIRunnable *runnableObject)
{
  // Automatically call stop() if thread was stared before, but the thread itself is not running any more.
  if ( getCurrentThreadState() == cTHREAD_STATE_STARTED_FLAG )
  {
    IasThreadResult const stopResult = stop();
    if ( stopResult != eIasThreadOk )
    {
      return eIasThreadAlreadyStarted;
    }
  }

  if ( !__sync_bool_compare_and_swap( &mThreadState, cTHREAD_STATE_INVALID, cTHREAD_STATE_IS_STARTING_FLAG) )
  {
    return eIasThreadAlreadyStarted;
  }

  // Overwrite runnable object member if a valid new one has been passed.
  if (NULL != runnableObject)
  {
    mRunnableObject = runnableObject;
  }

  IasThreadResult result = eIasThreadOk;
  if(mRunnableObject == NULL)
  {
    result = eIasThreadObjectInvalid;
  }

  mAssureRunning = assureRunning;

  bool destroyBarrier= false;
  if ((result == eIasThreadOk) && mAssureRunning)
  {
    if ( !mThreadStartedBarrierInitialized )
    {
      if (pthread_barrier_init(&(mThreadStartedBarrier), NULL, 2) != 0)
      {
        result = eIasCreateBarrierFailed;
      }
      else
      {
        destroyBarrier = true;
        mThreadStartedBarrierInitialized = true;
      }
    }
  }

  IasThreadResult commitSchedulingParametersResult = eIasThreadOk;
  if (result == eIasThreadOk)
  {
    pthread_attr_t pthread_attr_default;
    bool attributeInitialized = false;
    if (pthread_attr_init(&pthread_attr_default) != 0)
    {
      result = eIasInitAttributeFailed;
    }
    else
    {
      attributeInitialized = true;
    }

    if ((result == eIasThreadOk) && mStackSize != 0)
    {
      if (pthread_attr_setstacksize(&pthread_attr_default, mStackSize) != 0)
      {
        result = eIasInitAttributeFailed;
      }
    }

    if (result == eIasThreadOk)
    {
      mStartThreadResult = eIasThreadOk;
      mRunThreadResult = eIasThreadFailed;
      // Setting the thread state before creating the thread, because the run() method also accesses this variable.
      (void) __sync_or_and_fetch( &mThreadState, cTHREAD_STATE_STARTED_FLAG);
      if (pthread_create(&(mThreadId), &pthread_attr_default, run, (void*) this) != 0)
      {
        result = eIasCreateThreadFailed;
        // Reset the started flag.
        (void) __sync_and_and_fetch(&mThreadState, ~cTHREAD_STATE_STARTED_FLAG);
      }
      else
      {
        // Store this error code separately as we want to return it to the user,
        // however we do not want this error to prevent the creation of the thread.
        commitSchedulingParametersResult = commitSchedulingParameters(mThreadId, mSchedulingPolicy, mSchedulingPriority);
      }
    }

    if(attributeInitialized)
    {
      if (pthread_attr_destroy(&pthread_attr_default) != 0)
      {
        result = eIasDestroyAttributeFailed;
      }
    }
  }

  if ((result == eIasThreadOk) && mAssureRunning)
  {
    int32_t barrierWaitResult = pthread_barrier_wait(&(mThreadStartedBarrier));
    if (barrierWaitResult == PTHREAD_BARRIER_SERIAL_THREAD)
    {
      if (pthread_barrier_destroy(&(mThreadStartedBarrier)) != 0)
      {
        result = eIasDestroyBarrierFailed;
      }

      // There was an attempt to destroy barrier.
      mThreadStartedBarrierInitialized = false;
      destroyBarrier = false;
    }
    else if (barrierWaitResult != 0)
    {
      result = eIasWaitBarrierFailed;
    }
    else
    {
      // Thread should destroy barrier.
      destroyBarrier = false;
    }
  }

  if (destroyBarrier)
  {
    if (pthread_barrier_destroy(&(mThreadStartedBarrier)) != 0)
    {
      result = eIasDestroyBarrierFailed;
    }
    mThreadStartedBarrierInitialized = false;
  }

  if (result == eIasThreadOk)
  {
    result = mStartThreadResult;
  }
  else
  {
    (void) stop();
  }

  if (result == eIasThreadOk)
  {
    result = commitSchedulingParametersResult;
  }

  (void) __sync_and_and_fetch(&mThreadState, ~cTHREAD_STATE_IS_STARTING_FLAG);

  return result;
}

IasThreadResult IasThread::stop()
{
  if (mRunnableObject == NULL)
  {
    return eIasThreadObjectInvalid;
  }

  uint8_t const currentThreadState = getCurrentThreadState();
  if ( (currentThreadState == cTHREAD_STATE_INVALID )
      || (currentThreadState& (cTHREAD_STATE_IS_STARTING_FLAG|cTHREAD_STATE_IS_STOPPING_FLAG)))
  {
    return eIasThreadNotRunning;
  }

  uint8_t const oldThreadState = __sync_fetch_and_or( &mThreadState, cTHREAD_STATE_IS_STOPPING_FLAG);
  // Ensure that actually we have triggered the stop. This ensures that stop cannot be run twice.
  if ( oldThreadState & cTHREAD_STATE_IS_STOPPING_FLAG )
  {
    return eIasThreadNotRunning;
  }
  // Ensure that when we set the stopping flag, the starting flag was not just set.
  if ( oldThreadState & cTHREAD_STATE_IS_STARTING_FLAG )
  {
    (void) __sync_and_and_fetch(&mThreadState, ~cTHREAD_STATE_IS_STOPPING_FLAG);
    return eIasThreadNotRunning;
  }

  IasThreadResult result = eIasThreadOk;
  if ((result == eIasThreadOk) && ( oldThreadState & cTHREAD_STATE_RUNNING_FLAG ) )
  {
    if (mRunnableObject->shutDown() )
    {
      result = eIasThreadFailed;
    }
  }
  if ((result == eIasThreadOk) && ( oldThreadState & cTHREAD_STATE_STARTED_FLAG ) )
  {
    if (pthread_join(mThreadId, NULL) != 0)
    {
      result = eIasJoinThreadFailed;
    }
    else
    {
      (void) __sync_and_and_fetch(&mThreadState, ~cTHREAD_STATE_STARTED_FLAG);
      IAS_ASSERT(! (mThreadState & cTHREAD_STATE_RUNNING_FLAG) );
    }
  }

  (void) __sync_and_and_fetch(&mThreadState, ~cTHREAD_STATE_IS_STOPPING_FLAG);

  if ( result == eIasThreadOk )
  {
    IAS_ASSERT( getCurrentThreadState() == cTHREAD_STATE_INVALID );
    __sync_lock_test_and_set(&mThreadState, cTHREAD_STATE_INVALID);
  }

  return result;
}


void * IasThread::run(void * arg)
{
  IasThread * const pointerToThis = reinterpret_cast<IasThread *>(arg);
  //DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, "IasThread::run(void * arg");
  if (pointerToThis != NULL)
  {
    (void)pointerToThis->setThreadName();
    pointerToThis->run();
  }
  return NULL;
}


void IasThread::run()
{
  mStartThreadResult = eIasThreadOk;
  mRunThreadResult = eIasThreadFailed;
  if(mRunnableObject == NULL)
  {
    if (mStartThreadResult == eIasThreadOk)
    {
      mStartThreadResult = eIasThreadObjectInvalid;
    }
  }

  if (mStartThreadResult == eIasThreadOk)
  {
    (void) __sync_or_and_fetch( &mThreadState, cTHREAD_STATE_RUNNING_FLAG);
  }
  if (mAssureRunning)
  {
    int32_t barrierWaitResult = pthread_barrier_wait(&(mThreadStartedBarrier));
    if (barrierWaitResult == PTHREAD_BARRIER_SERIAL_THREAD)
    {
      if (pthread_barrier_destroy(&(mThreadStartedBarrier)) != 0)
      {
        if(mStartThreadResult == eIasThreadOk)
        {
          mStartThreadResult = eIasDestroyBarrierFailed;
        }
      }
      // There was an attempt to destroy barrier.
      mThreadStartedBarrierInitialized = false;
    }
    else if (barrierWaitResult != 0)
    {
      if(mStartThreadResult == eIasThreadOk)
      {
        mStartThreadResult = eIasWaitBarrierFailed;
      }
    }
  }

  if (mStartThreadResult == eIasThreadOk)
  {
    if( mRunnableObject->beforeRun())
    {
      mRunThreadResult = eIasThreadFailed;
    }
    else
    {
      mRunThreadResult = eIasThreadOk;
    }
    bool runWasCalled = false;
    if (mRunThreadResult == eIasThreadOk)
    {
      runWasCalled = true;
      uint8_t const currentThreadState = getCurrentThreadState();
      if ( (!(currentThreadState & cTHREAD_STATE_IS_STOPPING_FLAG))
          && (currentThreadState & cTHREAD_STATE_RUNNING_FLAG))
      {
        if (mRunnableObject->run())
        {
          mRunThreadResult =  eIasThreadFailed;
        }
      }
    }
    (void) __sync_and_and_fetch(&mThreadState, ~cTHREAD_STATE_RUNNING_FLAG);
    if ( runWasCalled )
    {
      IasThreadResult afterRunResult;
      if (mRunnableObject->afterRun())
      {
        afterRunResult = eIasThreadFailed;
      }
      if(afterRunResult != eIasThreadOk)
      {
        if(mRunThreadResult == eIasThreadOk)
        {
          mRunThreadResult = afterRunResult;
        }
      }
    }
  }

  (void) __sync_and_and_fetch(&mThreadState, ~cTHREAD_STATE_RUNNING_FLAG);

}

IasThreadResult IasThread::setThreadName()
{
  return setThreadName(mThreadId, mThreadName);
}

std::string IasThread::getName() //const
{
  std::string threadName;
  if (getThreadName(mThreadId, threadName) == eIasThreadOk)
  {
    return threadName;
  }
  return "";
}

}
