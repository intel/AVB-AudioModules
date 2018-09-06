#
# Copyright (C) 2018 Intel Corporation. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

set( AUDIO_COMMON_VERSION_MAJOR    2 )
set( AUDIO_COMMON_VERSION_MINOR    2 )
set( AUDIO_COMMON_VERSION_REVISION 0 )

include( CMakeLists.common.txt )

IasCreateComponentDocumentation( audio common ${AUDIO_COMMON_VERSION_MAJOR} ${AUDIO_COMMON_VERSION_MINOR} ${AUDIO_COMMON_VERSION_REVISION}
  common
)

IasCreateBbRecipeFile(audio ias-audio-common ${AUDIO_COMMON_VERSION_MAJOR} ${AUDIO_COMMON_VERSION_MINOR} ${AUDIO_COMMON_VERSION_REVISION}
  SUMMARY "Audio - Common"
  DESCRIPTION "Common Audio components"
  GROUP "Multimedia/Audio"
  YOCTO_LAYER meta-ias-audio
  YOCTO_RECIPE_FOLDER recipes-multimedia/common
  YOCTO_GEN_EXTRA_FLAGS NO_BINBB NO_SRC
  PACKAGE ias-audio-common
    PACKAGE_SUMMARY "Common Audio components"
    PACKAGE_DESCRIPTION "Common Audio components used by SmartXbar"
    PACKAGE_ENTITIES common
    PACKAGE_ENTITIES alsa_smartx_plugin
    PACKAGE_ENTITIES libasound_module_rate_smartx
    PACKAGE_BUILD_REQUIRES alsa-lib
    PACKAGE_BUILD_REQUIRES libsndfile1
    PACKAGE_BUILD_REQUIRES tbb
    PACKAGE_BUILD_REQUIRES boost
)
