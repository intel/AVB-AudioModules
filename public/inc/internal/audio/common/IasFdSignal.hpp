/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file IasFdSignal.hpp
 * @date 2016
 * @brief 
 */

#ifndef IASFDSIGNAL_HPP_
#define IASFDSIGNAL_HPP_

#include "internal/audio/common/IasAudioLogging.hpp"

namespace IasAudio {


/**
 * @brief This class contains the implementation for the signaling mechanism between
 * the SmartXbar and the user space application
 */
class __attribute__ ((visibility ("default"))) IasFdSignal
{
  public:
    /**
     * @brief The result type
     */
    enum IasResult
    {
      eIasOk,         //!< Method executed successfully
      eIasFailed      //!< Failed to execute method
    };

    /**
     * @brief The mode in which the fdsignal instance shall be opened
     */
    enum IasMode
    {
      eIasUndefined,    //!< Undefined
      eIasRead,         //!< The instance is only used for reading from the fifo
      eIasWrite         //!< The instance is only used for writing to the fifo
    };

    /**
     * @brief Constructor
     */
    IasFdSignal();

    /**
     * @brief Destructor
     */
    virtual ~IasFdSignal();

    /**
     * @brief Create the signaling object. This only has to be done by the server.
     *
     * @param[in] name The name to be used for the signaling object
     *
     * @returns The result of the call
     * @retval eIasOk Successfully created signaling object
     * @retval eIasFailed Failed to create signaling object
     */
    IasResult create(const std::string &name, const std::string &groupName = "ias_audio");

    /**
     * @brief Destroy the signaling object
     *
     * This method has to be called to destroy the signaling object after it is not used anymore.
     * It has only be used by the server, which created the signaling object.
     */
    void destroy();

    /**
     * @brief Open the signaling object in the mode specified.
     *
     * This method has to be used by the server and the client before being able to use
     * the signaling object. The server has to open the signaling object in write mode
     * and the client has to open the signaling object in read mode.
     *
     * @param[in] name The name of the signaling object to be opened
     * @param[in] mode The mode in which the signaling object shall be opened.
     *
     * @returns The result of the call
     * @retval eIasOk Successfully opened signaling object
     * @retval eIasFailed Failed to open signaling object
     */
    IasResult open(const std::string &name, IasMode mode);

    /**
     * @brief Close the signaling object.
     *
     * This method has to be used to close the signaling object. It has to be used by the server and the client
     * of the signaling object.
     */
    void close();

    /**
     * @brief Read one character from the signaling object
     *
     * @returns The result of the call
     * @retval eIasOk Successfully read one character from the signaling object
     * @retval eIasFailed Failed to read one character from the signaling object
     */
    IasResult read();

    /**
     * @brief Write one character to the signaling object
     *
     * @returns The result of the call
     * @retval eIasOk Successfully wrote one character to the signaling object
     * @retval eIasFailed Failed to write one character to the signaling object
     */
    IasResult write();

    /**
     * @brief Get the underlying file descriptor.
     *
     * The file descriptor is only valid after the signaling object was successfully opened before
     *
     * @returns The file descriptor of the signaling object. Returns -1 if the signaling object wasn't successfully opened before.
     */
    int32_t getFd() const;

  private:
    /**
     * @brief Replace characters from the name that might not be supported by the file system.
     *
     * Currently the ':' and the ',' characters are replaced by '_'.
     *
     * @param[in] name Name that has to be fixed
     *
     * @returns The fixed name
     */
    std::string fixName(const std::string &name);

    /**
     * @brief change the group owner of the fd signal fifo
     * @param name full name of the FdSignal fifo
     * @param groupName to set as the owner
     * @return The result of the call
     * @retval eIasOk Sucessfully change the group owner
     * @retval eIasFailed Failed to change the group owner
     */
    IasResult changeGroup(const std::string &name, const std::string &groupName);

    DltContext              *mLog;             //!< Dlt Log Context
    std::string              mName;            //!< Name of the fifo
    bool                     mCreated;         //!< Flag to check if the fifo was already created
    bool                     mOpened;          //!< Flag to check if the fifo was already opened
    int32_t                  mFd;              //!< File descriptor
    IasMode                  mMode;            //!< Open mode
    bool                     mWriteLogged;     //!< Flag to remember if a warning was already logged in the write method
};

} /* namespace IasAudio */

#endif /* IASFDSIGNAL_HPP_ */
