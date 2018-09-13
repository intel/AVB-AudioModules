/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAudioLogging.cpp
 * @date   2012
 * @brief  This is the implementation of the IasAudioLogging class.
 */

#include <sched.h>
#include "internal/audio/common/IasAudioLogging.hpp"


namespace IasAudio {

IasAudioLogging::IasAudioLogging()
{
  mMutex.lock();
  mDltContextMap.clear();
  mMutex.unlock();
}

IasAudioLogging::~IasAudioLogging()
{
  mMutex.lock();
  DltContextMap::iterator dltContextMapEntryIt;
  for(dltContextMapEntryIt = mDltContextMap.begin(); dltContextMapEntryIt != mDltContextMap.end(); ++dltContextMapEntryIt)
  {
    DltContextMapEntry * dltContextMapEntry = (*dltContextMapEntryIt).second;
    if (dltContextMapEntry != NULL)
    {
      if(dltContextMapEntry->getDltContext() != NULL)
      {
        DLT_UNREGISTER_CONTEXT(*dltContextMapEntry->getDltContext());
        delete(dltContextMapEntry->getDltContext());
      }
      delete dltContextMapEntry;
    }
  }
  mDltContextMap.clear();
  mMutex.unlock();

  DLT_UNREGISTER_APP();
}

IasAudioLogging* IasAudioLogging::audioLoggingInstance()
{
  static IasAudioLogging theInstance;
  return &theInstance;
}


IasAudioLogging::DltContextMap & IasAudioLogging::getMap()
{
  return audioLoggingInstance()->mDltContextMap;
}

std::mutex& IasAudioLogging::getMutex()
{
  return audioLoggingInstance()->mMutex;
}

void IasAudioLogging::registerDltApp(bool enableConsoleLog)
{
  DLT_REGISTER_APP("XBAR", "SmartXbar");
  DLT_VERBOSE_MODE();

  if(true == enableConsoleLog)
  {
    dlt_enable_local_print();
  }
}

DltContext* IasAudioLogging::registerDltContext(const std::string context_id,
                                                   const char * context_description)
{
  DltLogLevelType lloglevel;
  DltContext *newContext = NULL;

  DltContextMap::iterator dltContextMapEntryIt;
  getMutex().lock();
  dltContextMapEntryIt = getMap().find(context_id);
  if(dltContextMapEntryIt == getMap().end())
  {
    DLT_VERBOSE_MODE();
    DltContextMapEntry *dltContextMapEntry = new DltContextMapEntry();

    dltContextMapEntryIt = getMap().find("global");
    if(dltContextMapEntryIt != getMap().end())
    {
      DltContextMapEntry * tmpdltContextMapEntry = (*dltContextMapEntryIt).second;
      lloglevel = tmpdltContextMapEntry->getDltLogLevel();
    }
    else
    {
      lloglevel = dltContextMapEntry->getDltLogLevel();
    }

    newContext = new DltContext();
    dltContextMapEntry->setDltContext(newContext);

    // only log warnings and above, do not trace any messages
    DLT_REGISTER_CONTEXT_LL_TS( *newContext, context_id.c_str(),
                                context_description,
                                lloglevel,
                                dltContextMapEntry->getDltTraceStatus());
    getMap()[context_id] = dltContextMapEntry;
  }
  else
  {
    DltContextMapEntry * dltContextMapEntry = (*dltContextMapEntryIt).second;
    newContext = dltContextMapEntry->getDltContext();
    if(NULL == newContext)
    {
      DLT_VERBOSE_MODE();

      newContext = new DltContext();
      dltContextMapEntry->setDltContext(newContext);

      // only log warnings and above, do not trace any messages
      DLT_REGISTER_CONTEXT_LL_TS( *newContext, context_id.c_str(),
                                  context_description,
                                  dltContextMapEntry->getDltLogLevel(),
                                  dltContextMapEntry->getDltTraceStatus());
    }
  }
  getMutex().unlock();
  return newContext;
}

DltContext * IasAudioLogging::getDltContext(const std::string contextId)
{
  getMutex().lock();
  DltContextMap::iterator dltContextMapEntryIt;
  dltContextMapEntryIt = getMap().find(contextId.c_str());
  if(dltContextMapEntryIt != getMap().end())
  {
    DltContextMapEntry * dltContextMapEntry = (*dltContextMapEntryIt).second;
    getMutex().unlock();
    return(dltContextMapEntry->getDltContext());
  }
  else
  {
    getMutex().unlock();
    return registerDltContext(contextId, "dummy context description");
  }
}

void IasAudioLogging::addDltContextItem(const std::string contextId, DltLogLevelType loglevel, DltTraceStatusType tracestatus)
{
  DltContextMapEntry * dltContextMapEntry = new DltContextMapEntry();

  dltContextMapEntry->setDltLogLevel(loglevel);
  dltContextMapEntry->setDltTraceStatus(tracestatus);
  dltContextMapEntry->setDltContext(NULL);
  getMutex().lock();
  getMap()[contextId] = dltContextMapEntry;
  getMutex().unlock();
}


} // namespace IasAudio
