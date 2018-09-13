/*
 * @COPYRIGHT_TAG@
 */

/**
 * @file IasAudioIpcMessageContainer.hpp
 * @date Oct 5, 2015
 * @version 0.1
 * @brief Defines a the IPC Message Container.
 * WARNING: Use the IasAudioIpcProtocolHead.hpp and IasAudioIpcProtocolTail.hpp
 */

#ifndef IASAUDIOIPCMESSAGECONTAINER_HPP_
#define IASAUDIOIPCMESSAGECONTAINER_HPP_

#include "audio/common/IasAudioCommonTypes.hpp"

namespace IasAudio {

/**
 * IasIpcCrc 32 Bit value that can contain a CRC32 checksum.
 */
typedef uint32_t IasIpcCrc;

/**
 * Class contains the IasIpcMessage serialized.
 */
class IasIpcMessageContainer
{
public:
  /**
   * Loads a class in the data package. Creates CRC32 sum and store the data in the data struct.
   * The id will stored in the id member.
   *
   * @param toSend Package that should be stored inside.
   * @return Return false if package to big, true is good
   */
  template<class C>
  bool loadData(const C& toSend)
  {

    type = GET_IPC_ID(C);

    if(ias_safe_memcpy(data, IAS_AUDIO_IPC_MESSAGE_SIZE, &toSend, sizeof(C)))
    {
      return false;
    }

    boost::crc_32_type checksumGenerator;
    checksumGenerator.process_bytes(this, sizeof(IasIpcMessageContainer) - sizeof(IasIpcCrc));
    crc = checksumGenerator.checksum();

    return true;
  }

  /**
   * Gets the stored data. This function does not trigger a CRC check. Please use good before.
   *
   * @param toReceive Package that should be the destination of the internal data.
   * @return true if good, false if the structure does not match to the contained data.
   */
  template<class C>
  bool getData(C* toReceive)
  {
    //Note: The Queue class has to decide if the data should be dropped. (Check for good)
    if(GET_IPC_ID(C) == type)
    {
      if(ias_safe_memcpy(toReceive, sizeof(C), data, sizeof(C)))
      {
        return false;
      }
      return true;
    }
    else
    {
      return false;
    }
  }

  /**
   * Checks if the contained package is the same type.
   *
   * @param checkFor Package that should be compared
   * @return true on a match
   */
  template<class C>
  bool forecast()
  {
    return type == GET_IPC_ID(C);
  }

  /**
   * Calculates the CRC checksum and check if it is correct.
   *
   * @return true is good
   */
  bool good()
  {
    boost::crc_32_type result;
    result.process_bytes(this, sizeof(IasIpcMessageContainer) - sizeof(IasIpcCrc));
    return (crc == result.checksum());
  }

  IasIpcId type; ///<< Type of the package that is in data array.

private:
  char data[IAS_AUDIO_IPC_MESSAGE_SIZE]; ///< Data that contains the serialized structure.
  IasIpcCrc crc;                              ///< CRC32 Checksum from (type, data)

};

}

#endif /*IASAUDIOIPCMESSAGECONTAINER_HPP_*/
