/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file IasFdSignal.cpp
 * @date 2016
 * @brief
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <grp.h>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string/replace.hpp>
#include "internal/audio/common/IasFdSignal.hpp"


namespace IasAudio {

namespace fs = boost::filesystem;
namespace alg = boost::algorithm;

static const std::string cClassName = "IasFdSignal::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"
#define LOG_DEVICE "device=" + mName + ":"

#ifndef FD_SIGNAL_PATH
#define FD_SIGNAL_PATH "/run/smartx/"
#endif

static const std::string cRuntimeDir(FD_SIGNAL_PATH);

IasFdSignal::IasFdSignal()
  :mLog(IasAudioLogging::registerDltContext("SXC", "SmartX Client"))
  ,mName("")
  ,mCreated(false)
  ,mOpened(false)
  ,mFd(-1)
  ,mMode(eIasUndefined)
  ,mWriteLogged(false)
{
  //Nothing to do here
}

IasFdSignal::~IasFdSignal()
{
  if (mOpened == true)
  {
    close();
  }
  if (mCreated == true)
  {
    destroy();
  }
}

void IasFdSignal::destroy()
{
  if (mCreated == true)
  {
    std::string fullPath = cRuntimeDir + mName;
    if (fs::exists(fullPath) == true)
    {
      boost::system::error_code ec;
      if (fs::remove(fullPath, ec) == false)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Could not delete fifo", fullPath, ":", ec.message());
        return;
      }
      mCreated = false;
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "Successfully deleted fifo", fullPath);
    }
    else
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, "Fifo", fullPath, "does not exist");
    }
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Fifo wasn't created before");
  }
}

IasFdSignal::IasResult IasFdSignal::open(const std::string& name, IasMode mode)
{
  if (mOpened == true)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Fifo already opened");
    return eIasFailed;
  }
  if (mCreated == false)
  {
    // Not created yet, so fix and save the name
    mName = fixName(name);
  }
  else
  {
    // Already created, so check if the name matches
    if (mName.compare(fixName(name)) != 0)
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Creation name", mName, "does not match open name", name);
      return eIasFailed;
    }
  }
  std::string fullPath = cRuntimeDir + mName;
  std::string flagsStr;
  int32_t flags = O_NONBLOCK;
  if (mode == eIasRead)
  {
    flags |= O_RDONLY;
    flagsStr = "read";
  }
  else if (mode == eIasWrite)
  {
    // fifo has to be opened for read and write, because else opening will fail with error ENXIO when no one has opened
    // the other end of the pipe, see man fifo.
    flags |= O_RDWR;
    flagsStr = "write";
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Undefined open mode");
    return eIasFailed;
  }
  mMode = mode;
  mFd = ::open(fullPath.c_str(), flags, 0);
  if (mFd < 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error opening fifo", fullPath, ":", strerror(errno));
    return eIasFailed;
  }
  mOpened = true;
  DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "Successfully opened fifo", fullPath, "for", flagsStr, ":", flags, "fd:", mFd);
  return eIasOk;
}

void IasFdSignal::close()
{
  if (mFd != -1)
  {
    ::close(mFd);
    mOpened = false;
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "Successfully closed fifo");
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, "Fifo wasn't opened before");
  }
}

IasFdSignal::IasResult IasFdSignal::read()
{
  if (mMode == eIasRead)
  {
    if (mFd != -1)
    {
      // Read from FIFO unitl all bytes have been consumed.
      // After the last byte has been read, the ::read() function returns with -1.
      ssize_t bytesRead;
      ssize_t bytesCnt = 0;
      do
      {
        char buf;
        bytesRead = ::read(mFd, &buf, 1);
        bytesCnt += bytesRead;
      } while (bytesRead > 0);

      if (bytesCnt < 0)
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "Fifo empty", ":", bytesCnt, strerror(errno));
        return eIasOk;
      }
      else
      {
        DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, LOG_DEVICE, "Successfully read", bytesCnt+1, "from fifo");
        return eIasOk;
      }
    }
    else
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Fifo not open");
      return eIasFailed;
    }
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Fifo opened in wrong mode, nothing to read");
    return eIasFailed;
  }
}

IasFdSignal::IasResult IasFdSignal::write()
{
  if (mMode == eIasWrite)
  {
    if (mFd != -1)
    {
      char buf = 0;
      ssize_t bytesWritten = ::write(mFd, &buf, 1);
      if (bytesWritten < 0)
      {
        if (errno == EAGAIN)
        {
          if (mWriteLogged == false)
          {
            mWriteLogged = true;
            DLT_LOG_CXX(*mLog, DLT_LOG_WARN, LOG_PREFIX, LOG_DEVICE, "Fifo currently not available. Maybe other end did not open fifo, or fifo is full.");
          }
          else
          {
            DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, LOG_DEVICE, "Fifo currently not available. Maybe other end did not open fifo, or fifo is full.");
          }
          return eIasOk;
        }
        else
        {
          DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error writing to fifo", ":", strerror(errno));
          return eIasFailed;
        }
      }
      else
      {
        mWriteLogged = false;
        DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, LOG_DEVICE, "Successfully wrote", bytesWritten, "to fifo");
        return eIasOk;
      }
    }
    else
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Fifo not open");
      return eIasFailed;
    }
  }
  else
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Fifo opened in wrong mode, nothing to write");
    return eIasFailed;
  }
}

int32_t IasFdSignal::getFd() const
{
  return mFd;
}

std::string IasFdSignal::fixName(const std::string &name)
{
  std::string fixedName = name;
  // We want to create a fifo with the given name, so replace all possibly existing characters
  // not supported by the filesystem
  alg::replace_all(fixedName, ":", "_");
  alg::replace_all(fixedName, ",", "_");
  return fixedName;
}

IasFdSignal::IasResult IasFdSignal::create(const std::string &name, const std::string &groupName)
{
  // Create the runtime directory to host the named pipe (fifo)
  mName = fixName(name);
  if (fs::exists(cRuntimeDir) == false)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Directory", cRuntimeDir, "does not exist");
    return eIasFailed;
  }
  // Remember the current umask setting and change it to keep the write permission for the group
  mode_t prevMode = umask(S_IWOTH);
  std::string fullPath = cRuntimeDir + mName;
  int32_t result = mkfifo(fullPath.c_str(), 0660);
  umask(prevMode);
  if (result < 0)
  {
    if (errno == EEXIST)
    {
      if (changeGroup(fullPath, groupName) != eIasOk)
      {
        return eIasFailed;
      }
      mCreated = true;
      DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "Fifo", fullPath, "already exists");
      return eIasOk;
    }
    else
    {
      DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, LOG_DEVICE, "Error creating the fifo", fullPath, ":", strerror(errno));
      return eIasFailed;
    }
  }
  else
  {
    if (changeGroup(fullPath, groupName) != eIasOk)
    {
        return eIasFailed;
    }
    mCreated = true;
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "Successfully created fifo", fullPath);
    return eIasOk;
  }
}

IasFdSignal::IasResult IasFdSignal::changeGroup(const std::string &name, const std::string &groupName)
{
  struct group* groupInfo = getgrnam(groupName.c_str());
  if (groupInfo == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "unknown group", groupName);
    return eIasFailed;
  }
  if (chown(name.c_str(), -1, groupInfo->gr_gid) < 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_INFO, LOG_PREFIX, LOG_DEVICE, "chown for Fifo: ", name, " failed");
    return eIasFailed;
  }
  return eIasOk;
}

} /* namespace IasAudio */
