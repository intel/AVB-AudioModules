#
# Copyright (C) 2018 Intel Corporation.All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
#

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    $(LOCAL_EXPORT_C_INCLUDE_DIRS) \
    $(LOCAL_PATH)/../public/inc \
    $(LOCAL_PATH)/../private/inc \
    bionic

LOCAL_SRC_FILES := \
    ../private/src/smartx_test_support/shm_test_support_processes/IasPipeMessage.cpp \
    ../private/src/smartx_test_support/IasFftFunctions.cpp \
    ../private/src/smartx_test_support/IasRingBufferTestWriter.cpp \
    ../private/src/smartx_test_support/IasRingBufferTestReader.cpp \
    ../private/src/smartx_test_support/IasWavComp.cpp \
    ../private/src/smartx_test_support/IasFilterEstimator.cpp \
    ../private/src/smartx_test_support/IasThdnEstimator.cpp

LOCAL_MODULE := libias-audio-smartx_test_support
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_CLANG := true

LOCAL_SHARED_LIBRARIES:= \
    libasound \
    libias-audio-common \
    libias-android-pthread \
    libias-core_libraries-base \
    libias-core_libraries-foundation \
    libsndfile-1.0.27 \
    libdlt

LOCAL_STATIC_LIBRARIES:= \
    libboost \
    libboost_serialization

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) -msse -msse2 -frtti -fexceptions -Werror \
    -Wpointer-arith

include $(BUILD_SHARED_LIBRARY)


#######################################################################
# Client Application for shared memory test
#######################################################################
include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../public/inc/internal/audio \
    $(LOCAL_PATH)/../private/inc \
    bionic

LOCAL_SRC_FILES := \
    ../private/src/smartx_test_support/shm_test_support_processes/IasShmClientProcess.cpp

LOCAL_STATIC_LIBRARIES := \
    liblog \
    libtbb \
    libboost

LOCAL_SHARED_LIBRARIES := \
    libias-audio-common \
    libias-audio-smartx_test_support \
    libias-core_libraries-foundation \
    libias-core_libraries-test_support \
    libias-core_libraries-command_line_parser \
    libias-core_libraries-base \
    libdlt \
    libsndfile-1.0.27 \
    libasound

ifeq ("$(shell [ $(ANDROID_VERSION) -eq $(ANDROID_M) ] && echo true)", "true")
BOOST_SHARED_FOLDER := /mnt/eavb/misc/media/
else
BOOST_SHARED_FOLDER := /mnt/eavb/misc/audioserver/
endif

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) \
    -Wall -Wextra -Werror -Wpointer-arith \
    -frtti -fexceptions \
    -DBOOST_INTERPROCESS_SHARED_DIR_PATH=\"$(BOOST_SHARED_FOLDER)\"

LOCAL_MODULE := ias_shm_client_process
LOCAL_CLANG := true

include $(BUILD_EXECUTABLE)
