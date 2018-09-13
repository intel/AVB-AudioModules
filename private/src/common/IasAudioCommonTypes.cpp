/*
 * @COPYRIGHT_TAG@
 */
/**
 * @file   IasAudioCommonTypes.cpp
 * @date   2015
 * @brief
 */

#include "audio/common/IasAudioCommonTypes.hpp"


namespace IasAudio {

#define STRING_RETURN_CASE(name) case name: return std::string(#name); break
#define DEFAULT_STRING(name) default: return std::string(name)

__attribute__ ((visibility ("default"))) IasAudioCommonResult  __attribute__((warn_unused_result)) ias_safe_memcpy(void * dest, size_t dest_size, const void * source, size_t source_size)
{
  size_t copy_length = source_size;

  if ((NULL == dest) || (NULL == source))
  {
    return eIasResultInvalidParam;
  }

  if (((char*)source < ((char*)dest + dest_size)) &&  ((char*)dest < ((char*)source + source_size)))
  {
    return eIasResultMemoryError;
  }
  if (dest_size < source_size)
  {
    copy_length = dest_size;
  }

  (void)memcpy(dest, source,copy_length);

  return eIasResultOk;
}

__attribute__ ((visibility ("default"))) std::string toString(const IasAudioPinLinkType&  type)
{
  switch(type)
  {
    STRING_RETURN_CASE(IasAudioPinLinkType::eIasAudioPinLinkTypeImmediate);
    STRING_RETURN_CASE(IasAudioPinLinkType::eIasAudioPinLinkTypeDelayed);
    DEFAULT_STRING("IasAudioPinLinkType::eIasAudioPinLinkTypeInvalid");
  }
}

__attribute__ ((visibility ("default"))) std::string toString ( const IasAudioCommonDataLayout& type )
{
  switch(type)
  {
    STRING_RETURN_CASE(eIasLayoutNonInterleaved);
    STRING_RETURN_CASE(eIasLayoutInterleaved);
    STRING_RETURN_CASE(eIasLayoutUndef);
    DEFAULT_STRING("eIasLayoutInvalid");
  }
}

__attribute__ ((visibility ("default"))) std::string toString ( const IasAudioCommonAccess& type)
{
  switch(type)
  {
    STRING_RETURN_CASE(eIasAccessUndef);
    STRING_RETURN_CASE(eIasAccessMmap);
    STRING_RETURN_CASE(eIasAccessRw);
    DEFAULT_STRING("eIasAccessInvalid");
  }
}

__attribute__ ((visibility ("default"))) std::string toString ( const IasAudioCommonDataFormat& type )
{
  switch(type)
  {
    STRING_RETURN_CASE(eIasFormatFloat32);
    STRING_RETURN_CASE(eIasFormatInt16);
    STRING_RETURN_CASE(eIasFormatInt32);
    STRING_RETURN_CASE(eIasFormatUndef);
    DEFAULT_STRING("eIasFormatInvalid");
  }
}

__attribute__ ((visibility ("default"))) std::string toString(const IasPortDirection&  type)
{
  switch(type)
  {
    STRING_RETURN_CASE(eIasPortDirectionUndef);
    STRING_RETURN_CASE(eIasPortDirectionInput);
    STRING_RETURN_CASE(eIasPortDirectionOutput);
    DEFAULT_STRING("eIasPortDirectionInvalid");
  }
}

__attribute__ ((visibility ("default"))) std::string toString(const IasDeviceType&  type)
{
  switch(type)
  {
    STRING_RETURN_CASE(eIasDeviceTypeUndef);
    STRING_RETURN_CASE(eIasDeviceTypeSource);
    STRING_RETURN_CASE(eIasDeviceTypeSink);
    DEFAULT_STRING("eIasDeviceTypeInvalid");
  }
}


__attribute__ ((visibility ("default"))) int32_t toSize(const IasAudioCommonDataFormat& type)
{
  switch(type)
  {
    case eIasFormatFloat32:
    case eIasFormatInt32:
      return 4;
      break;
    case eIasFormatInt16:
      return 2;
      break;
    case eIasFormatUndef:
    default:
      return -1;
  }
}

__attribute__ ((visibility ("default"))) std::string toString ( const IasAudioCommonResult& type )
{
  switch(type)
  {
    STRING_RETURN_CASE(eIasResultOk);
    STRING_RETURN_CASE(eIasResultInitFailed);
    STRING_RETURN_CASE(eIasResultInvalidParam);
    STRING_RETURN_CASE(eIasResultNotInitialized);
    STRING_RETURN_CASE(eIasResultMemoryError);
    STRING_RETURN_CASE(eIasResultTimeOut);
    STRING_RETURN_CASE(eIasResultCondWaitMutexOwn);
    STRING_RETURN_CASE(eIasResultCondWaitParam);
    STRING_RETURN_CASE(eIasResultUnknown);
    STRING_RETURN_CASE(eIasResultObjectNotFound);
    STRING_RETURN_CASE(eIasResultClockDomainRate);
    STRING_RETURN_CASE(eIasResultInvalidSampleSize);
    STRING_RETURN_CASE(eIasResultObjectAlreadyExists);
    STRING_RETURN_CASE(eIasResultInvalidShmPath);
    STRING_RETURN_CASE(eIasResultBufferEmpty);
    STRING_RETURN_CASE(eIasResultBufferFull);
    STRING_RETURN_CASE(eIasResultAlreadyInitialized);
    STRING_RETURN_CASE(eIasResultNotAllowed);
    STRING_RETURN_CASE(eIasResultAlsaError);
    STRING_RETURN_CASE(eIasResultCRCError);
    STRING_RETURN_CASE(eIasResultInvalidSegmentSize);
    DEFAULT_STRING("eIasResultInvalid");
  }
}

__attribute__ ((visibility ("default"))) std::string toString ( const IasClockType& type )
{
  switch(type)
  {
    STRING_RETURN_CASE(eIasClockProvided);
    STRING_RETURN_CASE(eIasClockReceived);
    STRING_RETURN_CASE(eIasClockReceivedAsync);
    STRING_RETURN_CASE(eIasClockUndef);
    DEFAULT_STRING("eIasClockInvalid");
  }
}

__attribute__ ((visibility ("default"))) std::string toString ( const IasRingbufferType& type )
{
  switch(type)
  {
    STRING_RETURN_CASE(eIasRingBufferShared);
    STRING_RETURN_CASE(eIasRingBufferLocalMirror);
    STRING_RETURN_CASE(eIasRingBufferLocalReal);
    STRING_RETURN_CASE(eIasRingBufferUndef);
    DEFAULT_STRING("eIasRingBufferInvalid");
  }
}

__attribute__ ((visibility ("default"))) std::string toString ( const IasRingBufferAccess& type )
{
  switch(type)
  {
    STRING_RETURN_CASE(eIasRingBufferAccessRead);
    STRING_RETURN_CASE(eIasRingBufferAccessWrite);
    STRING_RETURN_CASE(eIasRingBufferAccessUndef);
    DEFAULT_STRING("eIasRingBufferAccessInvalid");
  }
}

__attribute__ ((visibility ("default"))) std::string toString ( const IasAudioArea& type )
{
  return std::string("IasAudioArea:") + " .start=" + std::to_string((unsigned long)type.start) +
         ", .first=" + std::to_string(type.first) + ", .step=" + std::to_string(type.step) +
         ", .index=" + std::to_string(type.index) + ", .maxIndex=" + std::to_string(type.maxIndex);
}

#undef STRING_RETURN_CASE
#undef DEFAULT_STRING

} // namespace IasAudio
