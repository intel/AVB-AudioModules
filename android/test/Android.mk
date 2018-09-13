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

#######################################################################
# AudioLogging test
#######################################################################

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := AudioLoggingTest.cpp

LOCAL_STATIC_LIBRARIES := \
    liblog

LOCAL_SHARED_LIBRARIES := \
    libias-audio-common \
    libias-core_libraries-foundation \
    libias-core_libraries-base \
    libdlt

LOCAL_CLANG := true
LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) -Wall -Wextra -Werror -Wpointer-arith

LOCAL_MODULE := audio_logging_test

include $(BUILD_NATIVE_TEST)


#######################################################################
# ringbuffer test
#######################################################################

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../../public/inc/internal/audio \
    $(LOCAL_PATH)/../../private/tst/ringbuffer/inc \
    bionic

LOCAL_SRC_FILES := \
    ../../private/tst/ringbuffer/src/IasRingBufferTest.cpp \
    ../../private/tst/ringbuffer/src/IasRingBufferTestMain.cpp

LOCAL_STATIC_LIBRARIES := \
    liblog \
    libtbb \
    libboost

LOCAL_SHARED_LIBRARIES := \
    libias-audio-common \
    libias-core_libraries-foundation \
    libias-core_libraries-test_support \
    libias-core_libraries-base \
    libdlt \
    libsndfile-1.0.27 \
    libasound

LOCAL_CLANG := true
NULL_ALSA_DEVICE_NAME := plughw_31_0

LOCAL_CFLAGS := \
    -Wall -Wextra -Werror -Wpointer-arith \
    -frtti -fexceptions \
    -DALSA_DEVICE_NAME=\"$(NULL_ALSA_DEVICE_NAME)\"

LOCAL_REQUIRED_MODULES := asound.conf

LOCAL_MODULE := ringbuffer_test

include $(BUILD_NATIVE_TEST)


#######################################################################
#  ringbuffer_factory test
#######################################################################

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../../public/inc/internal/audio \
    $(LOCAL_PATH)/../../private/tst/ringbuffer_factory/inc \
    bionic

LOCAL_SRC_FILES := \
    ../../private/tst/ringbuffer_factory/src/IasRingBufferFactoryTest.cpp \
    ../../private/tst/ringbuffer_factory/src/IasRingBufferFactoryTestMain.cpp

LOCAL_STATIC_LIBRARIES := \
    liblog \
    libtbb \
    libboost

LOCAL_SHARED_LIBRARIES := \
    libias-audio-common \
    libias-core_libraries-foundation \
    libias-core_libraries-test_support \
    libias-core_libraries-base \
    libdlt \
    libsndfile-1.0.27 \
    libasound

NULL_ALSA_DEVICE_NAME := plughw_31_0

LOCAL_CLANG := true
LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) -Wall -Wextra -Werror -Wpointer-arith \
    -frtti -fexceptions \
    -DALSA_DEVICE_NAME=\"$(NULL_ALSA_DEVICE_NAME)\"

LOCAL_REQUIRED_MODULES := asound.conf

LOCAL_MODULE := ringbuffer_factory_test

include $(BUILD_NATIVE_TEST)


#######################################################################
#  shm_test
#######################################################################

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../../public/inc/internal/audio \
    $(LOCAL_PATH)/../../private/inc \
    bionic

LOCAL_SRC_FILES := \
    ../../private/tst/shm_test/src/IasShmInterprocessTest.cpp

LOCAL_STATIC_LIBRARIES := \
    liblog \
    libtbb \
    libboost \
    libboost_iostreams

LOCAL_SHARED_LIBRARIES := \
    libias-audio-common \
    libias-audio-smartx_test_support \
    libias-core_libraries-foundation \
    libias-core_libraries-test_support \
    libias-core_libraries-base \
    libdlt \
    libsndfile-1.0.27 \
    libasound

CLIENT_PROCESS_PATH := /system/bin/
BOOST_SHARED_FOLDER := /system/usr/

LOCAL_CLANG := true
LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) \
    -Wall -Wextra -Werror -Wpointer-arith \
    -frtti -fexceptions \
    -DBOOST_INTERPROCESS_SHARED_DIR_PATH=\"$(BOOST_SHARED_FOLDER)\" \
    -DCLIENT_PROCESS_PATH=\"$(CLIENT_PROCESS_PATH)\"

LOCAL_MODULE := shm_test
LOCAL_REQUIRED_MODULES := ias_shm_client_process

include $(BUILD_NATIVE_TEST)

#######################################################################
#  alsa_type_conversion_test
#######################################################################

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../../public/inc/internal/audio \
    $(LOCAL_PATH)/../../private/inc \
    bionic

LOCAL_SRC_FILES := \
    ../../private/tst/alsa_type_conversion/src/IasAlsaTypeConversionTest.cpp \
    ../../private/tst/alsa_type_conversion/src/IasAlsaTypeConversionTestMain.cpp

LOCAL_STATIC_LIBRARIES := \
    liblog \
    libtbb \
    libboost \
    libboost_iostreams

LOCAL_CLANG := true
LOCAL_SHARED_LIBRARIES := \
    libias-audio-common \
    libias-audio-smartx_test_support \
    libias-core_libraries-foundation \
    libias-core_libraries-test_support \
    libias-core_libraries-base \
    libdlt \
    libsndfile-1.0.27 \
    libasound

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) \
    -Wall -Wextra -Werror -Wpointer-arith \
    -frtti -fexceptions

LOCAL_MODULE := alsa_type_conversion_test

include $(BUILD_NATIVE_TEST)


#######################################################################
#  audio_common_types_test
#######################################################################

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../../public/inc/internal/audio \
    $(LOCAL_PATH)/../../private/inc \
    bionic

LOCAL_CLANG := true
LOCAL_SRC_FILES := \
    ../../private/tst/audio_common_types/src/IasAudioCommonTypesTest.cpp \
    ../../private/tst/audio_common_types/src/IasAudioCommonTypesTestMain.cpp

LOCAL_STATIC_LIBRARIES := \
    liblog \
    libtbb \
    libboost \
    libboost_iostreams

LOCAL_SHARED_LIBRARIES := \
    libias-audio-common \
    libias-audio-smartx_test_support \
    libias-core_libraries-foundation \
    libias-core_libraries-test_support \
    libias-core_libraries-base \
    libdlt \
    libsndfile-1.0.27 \
    libasound

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) \
    -Wall -Wextra -Werror -Wpointer-arith \
    -frtti -fexceptions

LOCAL_MODULE := audio_common_types_test

include $(BUILD_NATIVE_TEST)


#######################################################################
#  data_probe_test
#######################################################################

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../../public/inc/internal/audio \
    $(LOCAL_PATH)/../../private/inc \
    bionic

LOCAL_CLANG := true
LOCAL_SRC_FILES := \
    ../../private/tst/data_probe/src/IasDataProbeTest.cpp

LOCAL_STATIC_LIBRARIES := \
    liblog \
    libtbb \
    libboost \
    libboost_iostreams

LOCAL_SHARED_LIBRARIES := \
    libias-audio-common \
    libias-audio-smartx_test_support \
    libias-core_libraries-foundation \
    libias-core_libraries-test_support \
    libias-core_libraries-base \
    libdlt \
    libsndfile-1.0.27 \
    libasound

INJECT_FILE_FOLDER := /sdcard/
EXTRACT_FILE_FOLDER := /sdcard/

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) \
    -Wall -Wextra -Werror -Wpointer-arith \
    -frtti -fexceptions \
    -DINJECT_FILE_FOLDER=\"$(INJECT_FILE_FOLDER)\" \
    -DEXTRACT_FILE_FOLDER=\"$(EXTRACT_FILE_FOLDER)\"

LOCAL_MODULE := data_probe_test

include $(BUILD_NATIVE_TEST)


#######################################################################
#  copy_audio_area_buffers_test
#######################################################################

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../../public/inc/internal/audio \
    $(LOCAL_PATH)/../../private/inc \
    bionic

LOCAL_SRC_FILES := \
    ../../private/tst/copy_audio_area_buffers/src/IasCopyAudioAreaBuffersTest.cpp

LOCAL_STATIC_LIBRARIES := \
    liblog \
    libtbb \
    libboost \
    libboost_iostreams

LOCAL_CLANG := true
LOCAL_SHARED_LIBRARIES := \
    libias-audio-common \
    libias-audio-smartx_test_support \
    libias-core_libraries-foundation \
    libias-core_libraries-test_support \
    libias-core_libraries-base \
    libdlt \
    libsndfile-1.0.27 \
    libasound

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) \
    -Wall -Wextra -Werror -Wpointer-arith \
    -frtti -fexceptions

LOCAL_MODULE := copy_audio_area_buffers_test

include $(BUILD_NATIVE_TEST)

#######################################################################
#  mutex_condvar_test
#######################################################################

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../../public/inc/internal/audio \
    $(LOCAL_PATH)/../../private/inc \
    bionic

LOCAL_SRC_FILES := \
    ../../private/tst/mutex_condvar/src/IasMutexCondvarTest.cpp

LOCAL_CLANG := true

LOCAL_SHARED_LIBRARIES := \
    libias-audio-common \
    libias-core_libraries-foundation \
    libias-core_libraries-test_support \
    libias-core_libraries-base \
    libdlt

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) \
    -Wall -Wextra -Werror -Wpointer-arith \
    -frtti -fexceptions

LOCAL_MODULE := mutex_condvar_test

include $(BUILD_NATIVE_TEST)

#######################################################################
#  mutex_condvar_error_test
#######################################################################

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../../public/inc/internal/audio \
    $(LOCAL_PATH)/../../private/inc \
    bionic

LOCAL_CLANG := true
LOCAL_SRC_FILES := \
    ../../private/tst/mutex_condvar_error/src/IasMutexCondvarErrorTest.cpp


LOCAL_SHARED_LIBRARIES := \
    libias-audio-common \
    libias-core_libraries-foundation \
    libias-core_libraries-test_support \
    libias-core_libraries-base \
    libdlt

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) \
    -Wall -Wextra -Werror -Wpointer-arith \
    -frtti -fexceptions

LOCAL_MODULE := mutex_condvar_error_test

include $(BUILD_NATIVE_TEST)

#######################################################################
#  src_wrapper_test
#######################################################################

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../../public/inc/internal/audio \
    $(LOCAL_PATH)/../../private/inc \
    bionic

LOCAL_SRC_FILES := \
    ../../private/tst/src_wrapper/src/IasSrcWrapperTest.cpp

LOCAL_STATIC_LIBRARIES := \
    libtbb

LOCAL_CLANG := true
LOCAL_SHARED_LIBRARIES := \
    libias-audio-common \
    libias-core_libraries-foundation \
    libias-core_libraries-test_support \
    libias-core_libraries-base \
    libdlt

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) \
    -Wall -Wextra -Werror -Wpointer-arith \
    -frtti -fexceptions

LOCAL_MODULE := src_wrapper_test

include $(BUILD_NATIVE_TEST)


#######################################################################
#  memoryallocation_test
#######################################################################

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../../public/inc/internal/audio \
    $(LOCAL_PATH)/../../private/inc \
    $(LOCAL_PATH)/../../private/tst/memoryallocation/inc \
    bionic

LOCAL_CLANG := true
LOCAL_SRC_FILES := \
    ../../private/tst/memoryallocation/src/IasMemAllocTest.cpp \
    ../../private/tst/memoryallocation/src/main.cpp

LOCAL_STATIC_LIBRARIES := \
    libboost \
    libboost_iostreams

LOCAL_SHARED_LIBRARIES := \
    libias-audio-common \
    libias-audio-smartx_test_support \
    libias-core_libraries-foundation \
    libias-core_libraries-test_support \
    libias-core_libraries-base \
    libdlt \
    libasound

CLIENT_PROCESS_PATH := /system/bin/
BOOST_SHARED_FOLDER := /system/usr/

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) \
    -Wall -Wextra -Werror -Wpointer-arith \
    -frtti -fexceptions \
    -DBOOST_INTERPROCESS_SHARED_DIR_PATH=\"$(BOOST_SHARED_FOLDER)\" \
    -DCLIENT_PROCESS_PATH=\"$(CLIENT_PROCESS_PATH)\"

LOCAL_MODULE := memoryallocation

include $(BUILD_NATIVE_TEST)

#######################################################################
#  fd_signal_test
#######################################################################
include $(CLEAR_VARS)

LOCAL_SHARED_LIBRARIES := \
    libias-audio-common \
    libias-audio-smartx_test_support \
    libias-core_libraries-foundation \
    libias-core_libraries-test_support \
    libias-core_libraries-base \
    libdlt

LOCAL_STATIC_LIBRARIES := \
    libboost

LOCAL_SRC_FILES := ../../private/tst/fdsignal/src/IasFdSignalTest.cpp

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) -frtti -fexceptions -DFD_SIGNAL_PATH=\"/mnt/eavb/misc/run/smartx/\"

LOCAL_MODULE := fdsignal_test

LOCAL_CLANG := true
include $(BUILD_NATIVE_TEST)

#######################################################################
#  fft_functions_test
#######################################################################

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../../public/inc/internal/audio \
    $(LOCAL_PATH)/../../private/inc \
    bionic

LOCAL_SRC_FILES := \
    ../../private/tst/fft_functions/src/IasFftFunctionsTest.cpp

LOCAL_STATIC_LIBRARIES := \
    liblog \
    libboost \
    libboost_iostreams

LOCAL_CLANG := true
LOCAL_SHARED_LIBRARIES := \
    libias-audio-common \
    libias-audio-smartx_test_support \
    libias-core_libraries-foundation \
    libias-core_libraries-test_support \
    libias-core_libraries-base \
    libdlt \
    libsndfile-1.0.27 \
    libasound

RESOURCES_PATH := /system/etc/

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) \
    -Wall -Wextra -Werror -Wpointer-arith \
    -frtti -fexceptions \
    -DRESOURCES_PATH=\"$(RESOURCES_PATH)\"

LOCAL_MODULE := fft_functions_test

LOCAL_REQUIRED_MODULES := \
    fftTestData.bin \
    fftSpectralData.bin

include $(BUILD_NATIVE_TEST)

#######################################################################
# fft_functions_test resources
#######################################################################
include $(CLEAR_VARS)
LOCAL_MODULE := fftTestData.bin
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES := ../../private/tst/fft_functions/resources/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := fftSpectralData.bin
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES := ../../private/tst/fft_functions/resources/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)


#######################################################################
#  filter_estimator_test
#######################################################################

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../../public/inc/internal/audio \
    $(LOCAL_PATH)/../../private/inc \
    bionic

LOCAL_SRC_FILES := \
    ../../private/tst/filter_estimator/src/IasFilterEstimatorTest.cpp

LOCAL_STATIC_LIBRARIES := \
    liblog \
    libboost \
    libboost_iostreams

LOCAL_CLANG := true
LOCAL_SHARED_LIBRARIES := \
    libias-audio-common \
    libias-audio-smartx_test_support \
    libias-core_libraries-foundation \
    libias-core_libraries-test_support \
    libias-core_libraries-base \
    libdlt \
    libsndfile-1.0.27 \
    libasound

RESOURCES_PATH := /system/etc/

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) \
    -Wall -Wextra -Werror -Wpointer-arith \
    -frtti -fexceptions \
    -DRESOURCES_PATH=\"$(RESOURCES_PATH)\"

LOCAL_MODULE := filter_estimator_test

LOCAL_REQUIRED_MODULES := \
    output.wav \
    input.wav

include $(BUILD_NATIVE_TEST)

#######################################################################
# filter_estimator_test resources
#######################################################################
include $(CLEAR_VARS)
LOCAL_MODULE := output.wav
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES := ../../private/tst/filter_estimator/resources/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := input.wav
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES := ../../private/tst/filter_estimator/resources/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)



#######################################################################
#  samplerateconverter_controller_test
#######################################################################

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../../public/inc/internal/audio \
    $(LOCAL_PATH)/../../private/inc \
    bionic

LOCAL_CLANG := true
LOCAL_SRC_FILES := \
    ../../private/tst/samplerateconverter_controller/src/IasSrcControllerGTest.cpp \
    ../../private/tst/samplerateconverter_controller/src/IasSrcControllerGTestMain.cpp

LOCAL_STATIC_LIBRARIES := \
    liblog \
    libtbb \
    libboost \
    libboost_iostreams

LOCAL_SHARED_LIBRARIES := \
    libias-audio-common \
    libias-audio-smartx_test_support \
    libias-core_libraries-foundation \
    libias-core_libraries-test_support \
    libias-core_libraries-base \
    libdlt \
    libsndfile-1.0.27 \
    libasound

NFS_PATH := /system/etc/

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) \
    -Wall -Wextra -Werror -Wpointer-arith \
    -frtti -fexceptions \
    -DNFS_PATH=\"$(NFS_PATH)\"

LOCAL_MODULE := samplerateconverter_controller_test

include $(BUILD_NATIVE_TEST)


#######################################################################
# asound.conf
#######################################################################
include $(CLEAR_VARS)
LOCAL_MODULE := asound.conf
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES := res/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)
