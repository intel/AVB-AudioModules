/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file   IasMetaData.hpp
 * @date   2015
 * @brief
 */

#ifndef IASMETADATA_HPP_
#define IASMETADATA_HPP_


namespace IasAudio {

class IasMetaData;
typedef std::vector<IasMetaData*> IasMetaDataVector;

struct IasMetaDataHeader
{
  static const uint32_t cMagicNumber = 0xABCD1234;   //!< A magic number

  /**
   * Constructor
   */
  IasMetaDataHeader()
    :mMagicNumber(cMagicNumber)
    ,mSize(sizeof(struct IasMetaDataHeader))
    ,mChannelLayout(0)
  {}

  uint32_t     mMagicNumber;     //!< Magic number to recognize correct initialization
  uint32_t     mSize;            //!< Size of the whole meta data including user defined members
  uint32_t     mChannelLayout;   //!< Channel layout of the stream contained in the buffer for which this meta data is managed
};

class __attribute__ ((visibility ("default"))) IasMetaData
{
  public:
    /**
     * @brief Constructor.
     */
    IasMetaData()
      :mAddr(NULL)
      ,mIndex(0)
      ,mMaxIndex(0)
    {}

    /**
     * @brief Destructor, virtual by default.
     */
    virtual ~IasMetaData()
    {}

    /**
     * @brief Get the address of the user meta data buffer
     *
     * It has to be casted to the correct struct to obtain
     * the content.
     * @return The address of the user meta data struct
     */
    inline void* getAddr() const { return mAddr; }

    /**
     * @brief Get the index of the user meta data buffer.
     *
     * Index count starts at zero.
     * @return The index of the buffer in the array.
     */
    inline uint32_t getIndex() const { return mIndex; }

    /**
     * @brief Get the maximum index of all user meta data buffers.
     *
     * Index count starts at zero.
     * @return The maximum index of the array.
     */
    inline uint32_t getMaxIndex() const { return mMaxIndex; }

    /**
     * @brief Initialize the members with the correct parameters
     * @param[in] addr The address of the user meta data buffer
     * @param[in] index The index of the user meta data buffer in the array
     * @param[in] maxIndex The maximum index of all user meta data buffers
     */
    inline void setParams(void *addr, uint32_t index, uint32_t maxIndex)
    {
      mAddr = addr;
      mIndex = index;
      mMaxIndex = maxIndex;
    }

  private:
    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasMetaData(IasMetaData const &other);

    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasMetaData& operator=(IasMetaData const &other);

    // Member variables
    void        *mAddr;            //!< The address of the user meta data buffer
    uint32_t     mIndex;           //!< The index of the user meta data buffer in the array
    uint32_t     mMaxIndex;        //!< The maximum index of all user meta data buffers
};

} //namespace IasAudio

#endif /* IASMETADATA_HPP_ */
