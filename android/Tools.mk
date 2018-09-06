#
# Copyright (C) 2018 Intel Corporation.All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../private/inc/tools

LOCAL_SRC_FILES := \
    ../private/src/tools/latency/IasAlsa.cpp \
    ../private/src/tools/latency/IasDelayMeasurement.cpp \
    ../private/src/tools/latency/IasLatency.cpp \
    ../private/src/tools/latency/main.cpp

LOCAL_STATIC_LIBRARIES := \
    libtbb \
    liblog

LOCAL_SHARED_LIBRARIES := \
    libias-audio-common \
    libias-core_libraries-foundation \
    libias-core_libraries-base \
    libdlt \
    libias-core_libraries-command_line_parser  \
    libcutils \
    libasound

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) -Wall -Wextra -Werror -Wno-unused-parameter

LOCAL_CLANG := true

LOCAL_MODULE := audio_latency_module_tools
LOCAL_CLANG := true

include $(BUILD_EXECUTABLE)
