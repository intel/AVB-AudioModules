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

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) -Wall -Wextra -Werror -Wpointer-arith

LOCAL_CLANG := true

LOCAL_MODULE := audio_latency_module_tools
LOCAL_CLANG := true

include $(BUILD_EXECUTABLE)
