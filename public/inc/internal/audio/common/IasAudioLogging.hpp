/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasAudioLogging.hpp
 * @date   2013
 * @brief  The definition of the IasAudioLogging class.
 */

#ifndef IASAUDIOLOGGING_HPP_
#define IASAUDIOLOGGING_HPP_

#include "dlt/dlt.h"
#include <dlt/dlt_cpp_extension.hpp>
//#include "audio/common/IasAudioCommonTypes.hpp"

using namespace std;

namespace IasAudio {

/**
 * @class IasAudioLogging
 *
 * This class combines commonly used audio chain parameters that every component of the audio chain would need.
 * An instance of this environment class exists once per audio chain.
 */
class __attribute__ ((visibility ("default"))) IasAudioLogging
{
  public:
    /**
     * @brief Destructor, virtual by default.
     */
    virtual ~IasAudioLogging();


    /*!
     * @brief Method to register the Application to dlt
     *
     * @returns void
     */
    static void registerDltApp(bool enableConsoleLog);


    /*!
     * @brief Getter method to get a dlt context by a given name
     *
     * @param[in] contextId The context id that you want to get
     * @returns The dlt context
     */
    static DltContext * getDltContext(const std::string contextId);

    /*!
     * @brief register a new dlt context
     *
     * @param[in] context_id Id of the context to register
     * @param[in] context_description Description to the context
     * @returns The dlt context
     */
    static DltContext* registerDltContext(const std::string context_id,
                                          const char * context_description);

    /*!
     * @ brief add a context to the list only with a id, log level and trace status. this is needed to set the
     * log level for specific context at startup. these levels will be sued to register the context afterwards
     * @param[in] contextId Id of the context to add
     * @param[in] loglevel loglevel of the context to add
     * @param[in] tracestatus tracestatus of the context to add
     * @returns void
     */
    static void addDltContextItem(const std::string contextId, DltLogLevelType loglevel, DltTraceStatusType tracestatus);

    /**
      * @brief Setter method for the log level
      *
      * @param[in] loglevel The global log level for the dlt.
      */
     static void setDltLogLevel(DltLogLevelType loglevel);

     /**
      * @brief Getter method for the log level
      *
      * @returns The global dlt log level.
      */
     static DltLogLevelType getDltLogLevel();

     /**
       * @brief Setter method for the trace status
       *
       * @param[in] tracestatus The global trace status for the dlt.
       */
      static void setDltTraceStatus(DltTraceStatusType tracestatus);

      /**
       * @brief Getter method for the log level
       *
       * @returns The global dlt log level.
       */
      static DltTraceStatusType getDltTraceStatus();


  private:

      /**
       * @brief Constructor.
       */
      IasAudioLogging();

      /*!
       * @brief Documentation for class DltContextMapEntry
       */
      class DltContextMapEntry
      {
        public:
          /*!
           *  @brief Constructor.
           */
          inline DltContextMapEntry()
          :mDltTraceStatus(DLT_TRACE_STATUS_OFF)
          ,mDltLogLevel(DLT_LOG_ERROR)
          ,mDltContext(NULL)
          {};

          /*!
           *  @brief Destructor, virtual by default.
           */
          virtual ~DltContextMapEntry(){};

          /*!
           * @brief set the dlt trace status
           * @param dltTraceStatus trace status
           */
          inline void setDltTraceStatus(DltTraceStatusType dltTraceStatus) { mDltTraceStatus = dltTraceStatus; }

          /*!
           *
           * @return The trace status
           */
          inline DltTraceStatusType getDltTraceStatus() const { return mDltTraceStatus; }


          /*!
            * @brief set the dlt log level
            * @param dltTraceStatus trace status
            */
           inline void setDltLogLevel(DltLogLevelType dltLogLevel) { mDltLogLevel = dltLogLevel; }

           /*!
            *
            * @return The log level
            */
           inline DltLogLevelType getDltLogLevel() const { return mDltLogLevel; }

           /*!
             * @brief set the dlt context
             * @param dltTraceStatus trace status
             */
            inline void setDltContext(DltContext * dltcontext) { mDltContext = dltcontext; }

            /*!
             *
             * @return The Dlt context
             */
            inline DltContext * getDltContext() const { return mDltContext; }


        private:
          /*!
           *  @brief Copy constructor, private unimplemented to prevent misuse.
           */
          DltContextMapEntry(DltContextMapEntry const &other);

          /*!
           *  @brief Assignment operator, private unimplemented to prevent misuse.
           */
          DltContextMapEntry& operator=(DltContextMapEntry const &other);

          DltTraceStatusType     mDltTraceStatus;   //!< the global trace status
          DltLogLevelType        mDltLogLevel;      //!< the global log level
          DltContext            *mDltContext;       //!< the Dlt logging context


      };
    typedef std::map<const std::string , DltContextMapEntry *> DltContextMap;

    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasAudioLogging(IasAudioLogging const &other);

    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasAudioLogging& operator=(IasAudioLogging const &other);

    static IasAudioLogging *audioLoggingInstance();

    // Member variables
    DltContextMap      mDltContextMap;   //!< A list of all Dlt contexts

    //instance
    static DltContextMap & getMap();

};

} //namespace IasAudio

#endif /* IASAUDIOLOGGING_HPP_ */
