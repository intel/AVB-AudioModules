#
# Copyright (C) 2018 Intel Corporation.All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
#

include $(CLEAR_VARS)

LOCAL_MODULE := libasound_module_rate_smartx
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_CLANG := true

LOCAL_SRC_FILES := ../private/src/samplerateconverter/IasAlsa.cpp

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../public/inc \
    $(LOCAL_PATH)/../private/inc/alsa_smartx_plugin \
    bionic

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/../public/inc/

LOCAL_STATIC_LIBRARIES := \
    libtbb

LOCAL_SHARED_LIBRARIES := \
    libias-android-pthread \
    libasound \
    libias-audio-common \
    libias-core_libraries-foundation \
    libias-core_libraries-base \
    libdlt

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) -frtti -fexceptions  -Werror -Wpointer-arith

include $(BUILD_SHARED_LIBRARY)
