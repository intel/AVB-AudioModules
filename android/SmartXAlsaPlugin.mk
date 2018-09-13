#
# INTEL CONFIDENTIAL
# Copyright (c) 2015-2017 Intel Corporation
# All Rights Reserved.
#
# The source code contained or described herein and all documents related to
# the source code ("Material") are owned by Intel Corporation or its
# suppliers or licensors. Title to the Material remains with Intel
# Corporation or its suppliers and licensors. The Material may contain trade
# secrets and proprietary and confidential information of Intel Corporation
# and its suppliers and licensors, and is protected by worldwide copyright
# and trade secret laws and treaty provisions. No part of the Material may be
# used, copied, reproduced, modified, published, uploaded, posted,
# transmitted, distributed, or disclosed in any way without Intel's prior
# express written permission.
#
# No license under any patent, copyright, trade secret or other intellectual
# property right is granted to or conferred upon you by disclosure or
# delivery of the Materials, either expressly, by implication, inducement,
# estoppel or otherwise. Any license under such intellectual property rights
# must be express and approved by Intel in writing.
#
# Unless otherwise agreed by Intel in writing, you may not remove or alter
# this notice or any other notice embedded in Materials by Intel or Intel's
# suppliers or licensors in any way.
#

include $(CLEAR_VARS)

LOCAL_MODULE := libasound_module_pcm_smartx
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_CLANG := true
LOCAL_HEADER_LIBRARIES += libutils_headers
LOCAL_REQUIRED_MODULES := \
    50-smartx.conf \
    asound.rc

LOCAL_SRC_FILES := \
    ../private/src/alsa_smartx_plugin/IasAlsaSmartXPlugin.cpp \
    ../private/src/alsa_smartx_plugin/IasAlsaSmartXConnector.cpp \

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../public/inc \
    $(LOCAL_PATH)/../private/inc/alsa_smartx_plugin \
    bionic

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/../public/inc/

LOCAL_STATIC_LIBRARIES := \
    libboost

LOCAL_SHARED_LIBRARIES := \
    libias-android-pthread \
    libasound \
    libias-audio-common \
    libias-core_libraries-foundation \
    libias-core_libraries-base \
    libdlt \
    liblog

OPEN_ONCE_LOCK_PATH := /run/smartx/

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) \
    -frtti -fexceptions -Wall -Wno-unused-parameter -Wpointer-arith \
    -fPIC -DPIC \
    -DOPEN_ONCE_LOCK_PATH=\"/mnt/eavb/misc$(OPEN_ONCE_LOCK_PATH)\"

ifeq ("$(shell [ $(ANDROID_VERSION) -eq $(ANDROID_M) ] && echo true)", "true")
BOOST_SHARED_FOLDER := /mnt/eavb/misc/media/
else
BOOST_SHARED_FOLDER := /mnt/eavb/misc/audioserver/
endif

LOCAL_LDFLAGS := -Wl,-no-undefined -DBOOST_INTERPROCESS_SHARED_DIR_PATH=\"$(BOOST_SHARED_FOLDER)\" \

include $(BUILD_SHARED_LIBRARY)

#######################################################################
# Build for configuration file
# IasInstallFiles( "public/res" "usr/share/alsa/alsa.conf.d" "" base 50-smartx.conf IS_ABSOLUTE_DESTINATION )
#######################################################################
include $(CLEAR_VARS)
LOCAL_MODULE := 50-smartx.conf
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/usr/share/alsa/alsa.conf.d
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES := ../public/res/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

#######################################################################
# asound.rc
#######################################################################
include $(CLEAR_VARS)
LOCAL_MODULE := asound.rc
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES := res/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

#######################################################################
# TESTS
#######################################################################

#######################################################################
# alsa_smartx_plugin_tst
#######################################################################
#include $(CLEAR_VARS)

#LOCAL_C_INCLUDES := \
#    $(LOCAL_PATH)/../public/inc \
#    $(LOCAL_PATH)/../private/inc \
#    bionic

#LOCAL_SRC_FILES := \
#    ../private/tst/alsa_smartx_plugin_tst/src/IasAlsaSmartXPluginTest.cpp

#LOCAL_STATIC_LIBRARIES := \
#    liblog \
#    libtbb \
#    libsndfile-1.0.27 \
#    libboost

#LOCAL_SHARED_LIBRARIES := \
#    libias-android-pthread \
#    libasound_module_pcm_smartx \
#    libias-audio-common \
#    libias-core_libraries-foundation \
#    libias-core_libraries-test_support \
#    libias-core_libraries-base \
#    libdlt \
#    libasound

#LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) -Wall -Wextra -Werror -Werror \
#    -Wpointer-arith -frtti -fexceptions

#LOCAL_CLANG := true
#LOCAL_MODULE := alsa_smartx_plugin_tst

#include $(BUILD_NATIVE_TEST)

#######################################################################
# alsa_smartx_connection_tst
#######################################################################

#include $(CLEAR_VARS)

#LOCAL_C_INCLUDES := \
#    $(LOCAL_PATH)/../public/inc \
#    $(LOCAL_PATH)/../private/inc \
#    bionic

#LOCAL_SRC_FILES := \
#    ../private/tst/alsa_smartx_connection_tst/src/IasAlsaSmartXConnectionTest.cpp

#LOCAL_STATIC_LIBRARIES := \
#    liblog \
#    libtbb \
#    libsndfile-1.0.27 \
#    libboost

#LOCAL_SHARED_LIBRARIES := \
#    libias-audio-common \
#    libias-core_libraries-foundation \
#    libias-core_libraries-test_support \
#    libias-core_libraries-base \
#    libdlt \
#    libasound

#LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) -Wall -Wextra -Werror -frtti \
#    -Wpointer-arith -fexceptions

#LOCAL_CLANG := true
#LOCAL_MODULE := alsa_smartx_connection_tst

#include $(BUILD_NATIVE_TEST)
