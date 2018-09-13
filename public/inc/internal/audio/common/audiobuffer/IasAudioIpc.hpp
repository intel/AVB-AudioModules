/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file IasAudioIpc.hpp
 * @date Sep 22, 2015
 * @version 0.2
 * @brief Defines a IPC Class with mutexing that can be stored in a shared memory.
 * WARNING: Use the IasAudioIpcProtocolHead.hpp and IasAudioIpcProtocolTail.hpp
 */

#ifndef IASAUDIOIPC_HPP_
#define IASAUDIOIPC_HPP_

/*
 * System
 */
#include <unistd.h>

/*
 * Boost
 */
#include <boost/lockfree/queue.hpp>
#include <boost/lockfree/policies.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
/*
 * Ias
 */
#include "audio/common/IasAudioCommonTypes.hpp"

namespace IasAudio {

/**
 * The Boost Lockfree queue with a fixed capacity and a contained type.
 */
template<class C, size_t S>
using IasShmStaticQueue = boost::lockfree::queue<C, boost::lockfree::fixed_sized<true>, boost::lockfree::capacity<S> >;
using IasShmMutex = boost::interprocess::interprocess_mutex;
using IasShmCondition = boost::interprocess::interprocess_condition;
using IasShmScopedLock = boost::interprocess::scoped_lock<IasShmMutex>;


#ifndef IAS_AUDIO_IPC_QUEUE_SIZE
#error The IAS_AUDIO_IPC_QUEUE_SIZE is not defined.
#endif


class IasAudioIpc
{
  public:
    /**
     * Constructor
     */
    IasAudioIpc()
      :tempMessageExist(false)
      ,queue()
    {}

    /**
     * Destructor
     */
    ~IasAudioIpc()
    {}

    /**
     * Pushes a package in the ipc queue.
     * The Package must be declared with an ID in the Protocol Header.
     *
     * @param sendPackage Reference to the container that should be send.
     * @return eIasResultOk if succeed.
     * @return eIasResultBufferFull on full buffer.
     * @return eIasResultInvalidSegmentSize if ias_safe_memcpy fails.
     */
    template<class C>
    IasAudioCommonResult push(const C& sendPackage)
    {
      if(tempSendMessage.loadData<C>(sendPackage)) // Try to serialize
      {
        IasShmScopedLock lock(newPackageMutex);

        if(queue.push(tempSendMessage)) // Try to push
        {
          // Notify waiting Client
          newPackageCondition.notify_one();

          return eIasResultOk;
        }
        else
        {
          return eIasResultBufferFull;
        }
      }
      else
      {
        return eIasResultInvalidSegmentSize;
      }
    }

    /**
     * Gets the a package out of the queue with a memcpy.
     * The Package must be declared with an ID in the Protocol Header.
     * This pop will not block. If it is called with a empty queue it return false.
     *
     * @param receivePackage Reference of the destination data structure.
     * @return eIasResultOk if succeed
     * @return eIasResultInvalidParam if the package type is not equal the one in the queue.
     * @return eIasResultBufferEmpty if there is no package.
     * @return eIasResultCRCError if a CRC Error occured.
     */
    template<class C>
    IasAudioCommonResult pop_noblock(C* receivePackage)
    {

      if(tempMessageExist) //Temporary Package exists
      {
        if(tempMessage.getData<C>(receivePackage)) // Check if the package is extractable
        {
          tempMessageExist = false;
          return eIasResultOk;
        }
        else
        {
          return eIasResultInvalidParam;
        }
      }
      else  // Get new package
      {
        if(!queue.empty())
        {
          queue.pop(tempMessage);

          if(tempMessage.good()) // CRC check
          {
            if(tempMessage.getData<C>(receivePackage)) //Check if the package is extractable
            {
              return eIasResultOk;
            }
            else // Store for later
            {
              tempMessageExist = true;
              return eIasResultInvalidParam;
            }
          }
          else
          {
            return eIasResultCRCError;
          }
        }
        else // Empty
        {
          return eIasResultBufferEmpty;
        }
      }
    }

    /**
     * Gets the a package out of the queue with a memcpy.
     * The Package must be declared with an ID in the Protocol Header.
     * This pop will not block. If it is called with a empty queue it return false.
     *
     * @param receivePackage Reference of the destination data structure.
     * @return eIasResultOk if succeed
     * @return eIasResultInvalidParam if the package type is not equal the one in the queue.
     * @return eIasResultBufferEmpty if there is no package.
     * @return eIasResultCRCError if a CRC Error occured.
     */
    template<class C>
    IasAudioCommonResult pop(C* receivePackage)
    {
      // Check if there is a new package.
      IasShmScopedLock lock(newPackageMutex);

      while(!packagesAvailable())
      {
        // Wait until there is a package
        newPackageCondition.wait(lock);
      }

      return pop_noblock<C>(receivePackage);
    }

    /**
     * @brief Waits until a package is available
     *
     * This function will block and wait until at least one package is available
     * or the wait is canceled by calling pthread_cancel.
     */
    void waitForPackage()
    {
      // Check if there is a new package.
      IasShmScopedLock lock(newPackageMutex);

      while(!packagesAvailable())
      {
        // Wait until there is a package
        newPackageCondition.wait(lock);
      }
    }

    /**
     * @brief Waits until a package is available
     *
     * @param[in] timeoutMSec Timeout waiting for the package in msec
     *
     * @retval eIasResultOk Package(s) available
     * @retval eIasResultTimeOut Timeout while waiting for messages
     */
    IasAudioCommonResult waitForPackage(uint32_t timeoutMSec)
    {
      // Check if there is a new package.
      IasShmScopedLock lock(newPackageMutex);

      while(!packagesAvailable())
      {
        // Wait until there is a package
        if(!newPackageCondition.timed_wait(lock, boost::posix_time::microsec_clock::universal_time()
          + boost::posix_time::millisec(timeoutMSec)))
        {
          return eIasResultTimeOut;
        }
      }
      return eIasResultOk;
    }

    /**
     * Gets the a package out of the queue with a memcpy.
     * The Package must be declared with an ID in the Protocol Header.
     * This pop will not block. If it is called with a empty queue it return false.
     *
     * @param receivePackage Reference of the destination data structure.
     * @param[in] timeoutMSec Relative time in milliseconds.
     * @return eIasResultOk if succeed
     * @return eIasResultInvalidParam if the package type is not equal the one in the queue.
     * @return eIasResultBufferEmpty if there is no package.
     * @return eIasResultCRCError if a CRC Error occured.
     * @return eIasResultTimeOut of the wait was too long.
     */
    template<class C>
    IasAudioCommonResult pop_timed_wait(C* receivePackage, uint32_t timeoutMSec)
    {
      // Check if there is a new package.
      IasShmScopedLock lock(newPackageMutex);

      while(!packagesAvailable())
      {
        // Wait until there is a package
        if(!newPackageCondition.timed_wait(lock, boost::posix_time::microsec_clock::universal_time()
                                           + boost::posix_time::millisec(timeoutMSec)))
        {
          return eIasResultTimeOut;
        }
      }

      return pop_noblock<C>(receivePackage);
    }

    /**
     * Dry run of pop. No data will be copied or extracted out of the queue.
     * This function is non blocking.
     *
     * @param receivePackage Reference of the destination data structure.
     * @return eIasResultOk if succeed
     * @return eIasResultInvalidParam if the package type is not equal the one in the queue.
     * @return eIasResultBufferEmpty if there is no package.
     * @return eIasResultCRCError if a CRC Error occured.
     */
    template<class C>
    IasAudioCommonResult peek()
    {
      if(tempMessageExist) //Temporary Package exists
      {
        if(tempMessage.forecast<C>()) // Check if the package is extractable
        {
          return eIasResultOk;
        }
        else
        {
          return eIasResultInvalidParam;
        }

      }
      else  // Get new package
      {
        if(!queue.empty())
        {
          queue.pop(tempMessage);

          if(tempMessage.good()) // CRC check
          {
            if(tempMessage.forecast<C>()) //Check if the package is extractable
            {
              tempMessageExist = true;
              return eIasResultOk;
            }
            else // Store for later
            {
              tempMessageExist = true;
              return eIasResultInvalidParam;
            }

          }
          else
          {
            return eIasResultCRCError;
          }
        }
        else // Empty
        {
          return eIasResultBufferEmpty;
        }
      }
    }

    /**
     * Function gets next id.
     *
     * @return ID, 0 if failed.
     */
    IasIpcId getNextId()
    {
      if(tempMessageExist)
      {
        return tempMessage.type;
      }
      else
      {
        if(queue.pop(tempMessage))
        {
          if(tempMessage.good())
          {
            tempMessageExist = true;
            return tempMessage.type;
          }
          else
          {
            return 0;
          }
        }
        else
        {
          return 0;
        }
      }
    }

    /**
     * Gets if there are any packages remaining that can be fetched.
     *
     * @return True if there is a package available.
     */
    bool packagesAvailable()
    {
      return !queue.empty() || tempMessageExist;
    }

    /**
     * Discards the next package if available.
     *
     * @return eIasResultOk if succeed
     * @return eIasResultBufferEmpty if there was no package.
     */
    IasAudioCommonResult discardNext()
    {
      if(tempMessageExist)
      {
        tempMessageExist = false;
      }
      else if(!queue.empty())
      {
        queue.pop(tempMessage);
      }
      else
      {
        // No Packages in queue
        return eIasResultBufferEmpty;
      }
      return eIasResultOk;
    }

    /**
     * Discards all packages if available.
     *
     * @return eIasResultOk if succeed
     * @return eIasResultBufferEmpty if there was no package.
     */
    IasAudioCommonResult discardAll()
    {
      if(tempMessageExist || !queue.empty())
      {
        if(tempMessageExist)
        {
          tempMessageExist = false;
        }
        if(!queue.empty())
        {
          // Drop all elements
          queue.consume_all( [](IasIpcMessageContainer& drop){(void)drop;} );
        }
      }
      else
      {
        // No Packages in queue
        return eIasResultBufferEmpty;
      }
      return eIasResultOk;
    }

    /**
     * !!!
     * This function is for statistics only, the result is not reliable.
     * (This is a BOOST Library limitation of boost::lockfree::queue)
     *
     * @return Number of packages.
     */

  private:
    bool tempMessageExist;             ///< True if there is a prefetched package
    IasIpcMessageContainer tempMessage;     ///< Prefetched package
    IasIpcMessageContainer tempSendMessage; ///< Temporary package, that stores the data.
    IasShmMutex newPackageMutex;            ///< Mutex for the new package condition
    IasShmCondition newPackageCondition;    ///< New package condition.
    IasShmStaticQueue<IasIpcMessageContainer, IAS_AUDIO_IPC_QUEUE_SIZE> queue; ///< Queue that stores the data.

};

}


#endif /* IASAUDIOIPC_HPP_ */
