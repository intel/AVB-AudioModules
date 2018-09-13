/*
 * @COPYRIGHT_TAG@
 */
/**
 * @file   IasAlsaTypeConversion.cpp
 * @date   Oct 2, 2015
 * @version 0.1
 * @brief  Connector that connects the Alsa callback functions with the
 * provided smartx ring buffers
 */

#include "internal/audio/common/IasAlsaTypeConversion.hpp"
#include "internal/audio/common/IasAudioLogging.hpp"


namespace IasAudio {

static const std::string cClassName = "IasAlsaTypeConversion::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"


unsigned int convertChannelCountIasToAlsa(const IasAudioCommonChannelCount& in)
{
  return in;
}


IasAudioCommonChannelCount convertChannelCountAlsaToIas ( const unsigned int& in )
{
  return in;
}


snd_pcm_format_t convertFormatIasToAlsa(const IasAudioCommonDataFormat& in)
{
  switch(in)
  {
    case eIasFormatFloat32:
      return SND_PCM_FORMAT_FLOAT_LE;
      break;
    case eIasFormatInt16:
      return SND_PCM_FORMAT_S16_LE;
      break;
    case eIasFormatInt32:
      return SND_PCM_FORMAT_S32_LE;
      break;
    default:
      DltContext *logCtx = IasAudioLogging::registerDltContext("SXP", "SmartX Plugin");
      DLT_LOG_CXX(*logCtx, DLT_LOG_ERROR, LOG_PREFIX, "Invalid data format:", static_cast<uint32_t>(in));
      return SND_PCM_FORMAT_UNKNOWN;
  }
}


IasAudioCommonDataFormat convertFormatAlsaToIas(const snd_pcm_format_t& in)
{
  switch(in)
  {
    case SND_PCM_FORMAT_FLOAT_LE:
      return eIasFormatFloat32;
      break;
    case SND_PCM_FORMAT_S16_LE:
      return eIasFormatInt16;
      break;
    case SND_PCM_FORMAT_S32_LE:
      return eIasFormatInt32;
      break;
    default:
      DltContext *logCtx = IasAudioLogging::registerDltContext("SXP", "SmartX Plugin");
      DLT_LOG_CXX(*logCtx, DLT_LOG_ERROR, LOG_PREFIX, "Invalid data format:", static_cast<uint32_t>(in));
      return eIasFormatUndef;
  }
}


snd_pcm_access_t convertAccessTypeIasToAlsa(const IasAudioCommonDataLayout& in, const IasAudioCommonAccess& isMmap)
{
  if(isMmap == eIasAccessRw)
  {
    switch(in)
    {
      case eIasLayoutInterleaved:
        return SND_PCM_ACCESS_RW_INTERLEAVED;
        break;
      case eIasLayoutNonInterleaved:
        return SND_PCM_ACCESS_RW_NONINTERLEAVED;
      default:
        DltContext *logCtx = IasAudioLogging::registerDltContext("SXP", "SmartX Plugin");
        DLT_LOG_CXX(*logCtx, DLT_LOG_ERROR, LOG_PREFIX, "Invalid data layout:", static_cast<uint32_t>(in));
        return SND_PCM_ACCESS_LAST;
    }
  }
  else
  {
    switch(in)
    {
      case eIasLayoutInterleaved:
        return SND_PCM_ACCESS_MMAP_INTERLEAVED;
        break;
      case eIasLayoutNonInterleaved:
        return SND_PCM_ACCESS_MMAP_NONINTERLEAVED;
      default:
        DltContext *logCtx = IasAudioLogging::registerDltContext("SXP", "SmartX Plugin");
        DLT_LOG_CXX(*logCtx, DLT_LOG_ERROR, LOG_PREFIX, "Invalid data layout:", static_cast<uint32_t>(in));
        return SND_PCM_ACCESS_LAST;
    }
  }
}


snd_pcm_access_t convertAccessTypeIasToAlsa(const IasAudioCommonDataLayout& in)
{
  return convertAccessTypeIasToAlsa(in, eIasAccessRw);
}


void convertAreaIasToAlsa(const IasAudioArea& in, snd_pcm_channel_area_t* out)
{
  if(out)
  {
    out->addr = in.start;
    out->first = in.first;
    out->step = in.step;
  }
  else
  {
    DltContext *logCtx = IasAudioLogging::registerDltContext("SXP", "SmartX Plugin");
    DLT_LOG_CXX(*logCtx, DLT_LOG_ERROR, LOG_PREFIX, "out == nullptr");
  }
}

}
