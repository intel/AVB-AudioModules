/*
 * @COPYRIGHT_TAG@
 */

/**
 * @file IasAlsaPluginShmConnection.hpp
 * @date Oct 7, 2015
 * @version 0.1
 * @brief Defines a strucuture that contains all the modules that
 * are needed for a connection between the smartx and the smartx Plugin.
 *
 */
#ifndef IAS_ALSAPLUGIN_SHMCONNECTION_HPP_
#define IAS_ALSAPLUGIN_SHMCONNECTION_HPP_

#include "audio/common/IasAudioCommonTypes.hpp"
#include "internal/audio/common/IasFdSignal.hpp"


namespace IasAudio
{

struct IasAlsaHwConstraintsStatic;
class IasAudioIpc;
class IasMemoryAllocator;
class IasAudioRingBuffer;

class __attribute__ ((visibility ("default"))) IasAlsaPluginShmConnection
{
  public:
    /**
     * @brief Constructor inits the struct with invalid pointers.
     *
     */
    IasAlsaPluginShmConnection();

    /**
     * @brief Destructor, will delete the allocator if the class was the creator of it.
     * Will delete the AudioBuffer too if it was the creator.
     */
    ~IasAlsaPluginShmConnection();

    /**
     * @brief Creates a connection for offering to the IasAlsaSmartXConnector. Creates all needed structures.
     *
     * The group id of the shared memory files created in /dev/shm will be set to ias_audio as default value. It can
     * be overwritten to any valid group name of the executing process. In case the group does not exist, the creation
     * will fail.
     *
     * @param[in] connectionName Name of the connection, will be refered by the asound configuration of the smartx plugin.
     * @param[in] groupName Name of the group id of the created shared memory files in /dev/shm
     * @return IasAudio::IasAudioCommonResult cOk is good.
     */
    IasAudioCommonResult createConnection ( const std::string& connectionName, const std::string& groupName = "ias_audio" );

    /**
     * @brief Function to replace the old ring buffer or create a new one with the given parameters.
     *
     * @param[in] configStruct Configuration of the ring buffer. Not all values are needed only
     * period size, period number, channel number and format is used.
     * @return IasAudio::IasAudioCommonResult IasResultOk if good
     * @return IasAudio::IasAudioCommonResult Results are returned from the factory create function.
     */
    IasAudioCommonResult createRingBuffer ( const IasAudioDeviceParamsPtr configStruct );

    /**
     * @brief Function to search for a created connection.
     *
     * @param[in] connectionName Reference name for the connection.
     * @return IasAudio::IasAudioCommonResult
     */
    IasAudioCommonResult findConnection ( const std::string& connectionName );

    /**
     * @brief The connection gets destroyed if the class gets destructed.
     *
     */
    //IasAudioCommonResult destroyConnection(const std::string& connectionName);

    /**
     * @brief Gets a pointer to the IPC connection. This IPC connection is
     * unidirectional. If the Structure was initialized with create the connection is
     * from smartx plugin to smartx. When initialized with find, the returned ipc is smartx
     * to smartx plugin.
     *
     * @return IasAudioIpc* Pointer to the IPC queue.
     */
    inline IasAudioIpc* getInIpc()
    {
      return mInIpc;
    }

    /**
     * @brief  Gets a pointer to the IPC connection. This IPC connection is
     * unidirectional. If the Structure was initialized with create the connection is
     * from smartx to smartx plugin. When initialized with find, the returned ipc is
     * smartx plugin to smartx.
     *
     * @return IasAudioIpc* Pointer to the IPC queue.
     */
    inline IasAudioIpc* getOutIpc()
    {
      return mOutIpc;
    }

    /**
     * @brief Get a pointer to the hardware constraints, that belongs to this
     * connection.
     *
     * @return IasAlsaHwConstraintsStatic* Ponter to the Hardware Constraints structure.
     */
    inline IasAlsaHwConstraintsStatic* getAlsaHwConstraints()
    {
      return mConstraints;
    }

    /**
     * @brief Verify if the update Flag is set.
     * Gets a pointer to the RingBuffer holding the sample areas for the different channels.
     *
     * @return IasAudioRingBuffer* Pointer to the RingBuffer Instance.
     */
    IasAudioRingBuffer* verifyAndGetRingBuffer();

    /**
     * @brief Gets a pointer to the RingBuffer holding the sample areas for the different channels.
     *
     * @return IasAudioRingBuffer* Pointer to the RingBuffer Instance.
     */
    inline IasAudioRingBuffer* getRingBuffer()
    {
      return mRingBuffer;
    }

  private:

    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasAlsaPluginShmConnection(IasAlsaPluginShmConnection const &other);

    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasAlsaPluginShmConnection& operator=(IasAlsaPluginShmConnection const &other);


    DltContext  *mLog;                        //!< DLT Log context
    IasAudioIpc *mInIpc;                      //!< IPC Connection from smartx to the alsa plugin.
    IasAudioIpc *mOutIpc;                     //!< IPC Connection from the alsa plugin to the smartx.
    IasAlsaHwConstraintsStatic *mConstraints; //!< Contains the Hardware constrains that are read by the Plugin. Valid flag must be set in the constraints.
    std::string mRingBufferName;              //!< Name of the Ringbuffer
    IasAudioRingBuffer *mRingBuffer;          //!< Instance of a Ringbuffer.
    bool *mUpdateAvailable;              //!< Pointer to a central flag that marks if there was a change of the ringbuffer
    IasMemoryAllocator *mAllocator;           //!< Allocator that allocates or find the instances of the connection.
    bool mIsCreator;                     //!< Bool flag if the class was the creator of the resources. If true, the class will delete the shm region in the destructor.
    IasFdSignal mFdSignal;                    //!< Signal buffer level changes using file descriptors.
    std::string mConnectionName;              //!< The connection name.
    std::string mGroupName;                   //!< The group name.
};

}

#endif /* IAS_ALSAPLUGIN_SHMCONNECTION_HPP_ */
