/*
 * @COPYRIGHT_TAG@
 */
/**
 * @file IasCommonVersion.cpp
 * @date 2017
 * @brief Get the version string of the ias-audio-common lib
 */

#include "internal/audio/common/IasCommonVersion.hpp"
#include "version.h"

namespace IasAudio {

const char* getLibCommonVersion()
{
  return VERSION_STRING;
}


} /* namespace IasAudio */
