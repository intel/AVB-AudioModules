# Copyright (C) 2018 Intel Corporation. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
CMAKE_MINIMUM_REQUIRED( VERSION 3.4.3 )
project (AudioCommon)

# set release version for audio-common
set( AUDIO_COMMON_VERSION_MAJOR    4 )
set( AUDIO_COMMON_VERSION_MINOR    1 )
set( AUDIO_COMMON_VERSION_REVISION 0 )

set( AUDIO_COMMON_VERSION_STRING ${AUDIO_COMMON_VERSION_MAJOR}.${AUDIO_COMMON_VERSION_MINOR}.${AUDIO_COMMON_VERSION_REVISION})

# create a version.h file
set(CURRENT_VERSION_MAJOR ${AUDIO_COMMON_VERSION_MAJOR})
set(CURRENT_VERSION_MINOR ${AUDIO_COMMON_VERSION_MINOR})
set(CURRENT_VERSION_REVISION ${AUDIO_COMMON_VERSION_REVISION})
set(VERSION_H_INCLUDE_GUARD "AUDIO_COMMON_VERSION_H")
string(TOUPPER ${VERSION_H_INCLUDE_GUARD} VERSION_H_INCLUDE_GUARD)
string(REGEX REPLACE "[^a-zA-Z0-9_]" "_" VERSION_H_INCLUDE_GUARD ${VERSION_H_INCLUDE_GUARD})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/private/inc/helper/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)
configure_file(${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.pc.in ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.pc @ONLY)
unset(CURRENT_VERSION_MAJOR)
unset(CURRENT_VERSION_MINOR)
unset(CURRENT_VERSION_REVISION)
unset(VERSION_H_INCLUDE_GUARD)

if( NOT CMAKE_INSTALL_LIBDIR )
  # This can be overwritten externally by defining it to lib64 if required
  set( CMAKE_INSTALL_LIBDIR lib )
  message( "CMAKE_INSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}" )
endif()

install(FILES ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig COMPONENT dev)

include( CMakeLists.common.txt )

# include( CMakeLists.common_internal.txt )
# 
# IasCreateComponentDocumentation( audio common ${AUDIO_COMMON_VERSION_MAJOR} ${AUDIO_COMMON_VERSION_MINOR} ${AUDIO_COMMON_VERSION_REVISION}
#   common
# )
# 
# if( NOT IAS_DISABLE_TESTS )
#   set( ADDITIONAL_RECIPE_CONTENT
#   PACKAGE ias-audio-common-test-support
#     PACKAGE_SUMMARY "Test support packages"
#     PACKAGE_DESCRIPTION "Test support packages used by SmartXbar"
#     PACKAGE_ENTITIES smartx_test_support
#     PACKAGE_ENTITIES ias_shm_client_process
#     PACKAGE_ENTITIES latency
#   )
# endif()
# 
# IasCreateBbRecipeFile(audio ias-audio-common ${AUDIO_COMMON_VERSION_MAJOR} ${AUDIO_COMMON_VERSION_MINOR} ${AUDIO_COMMON_VERSION_REVISION}
#   SUMMARY "Audio - Common"
#   DESCRIPTION "Common Audio components"
#   GROUP "Multimedia/Audio"
#   YOCTO_LAYER meta-ias-audio
#   YOCTO_RECIPE_FOLDER recipes-multimedia/common
#   #YOCTO_GEN_EXTRA_FLAGS SOURCE_EXPORT
#   PACKAGE ias-audio-common
#     PACKAGE_SUMMARY "Common Audio components"
#     PACKAGE_DESCRIPTION "Common Audio components used by SmartXbar"
#     PACKAGE_ENTITIES common
#     PACKAGE_ENTITIES alsa_smartx_plugin
#     PACKAGE_ENTITIES libasound_module_rate_smartx
#     PACKAGE_BUILD_REQUIRES alsa-lib
#     PACKAGE_BUILD_REQUIRES libsndfile1
#     PACKAGE_BUILD_REQUIRES tbb
#     PACKAGE_BUILD_REQUIRES boost
#     PACKAGE_BUILD_REQUIRES dlt-daemon
#   ${ADDITIONAL_RECIPE_CONTENT}
# )
