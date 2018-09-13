# Copyright (C) 2018 Intel Corporation.All rights reserved.

# SPDX-License-Identifier: BSD-3-Clause

include $(CLEAR_VARS)

LOCAL_MODULE := libias-audio-common
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_CLANG := true
LOCAL_HEADER_LIBRARIES += libutils_headers
LOCAL_SRC_FILES := \
    ../private/src/common/IasAlsaTypeConversion.cpp \
    ../private/src/common/IasAudioCommonTypes.cpp \
    ../private/src/common/IasAudioLogging.cpp \
    ../private/src/common/IasDataProbe.cpp \
    ../private/src/common/IasDataProbeHelper.cpp \
    ../private/src/common/IasIntProcCondVar.cpp \
    ../private/src/common/IasIntProcMutex.cpp \
    ../private/src/common/IasFdSignal.cpp \
    ../private/src/common/IasCommonVersion.cpp

LOCAL_SRC_FILES += \
    ../private/src/alsa_smartx_plugin/IasAlsaPluginShmConnection.cpp

LOCAL_SRC_FILES += \
    ../private/src/audiobuffer/IasMemoryAllocator.cpp \
    ../private/src/audiobuffer/IasMetaDataFactory.cpp \
    ../private/src/audiobuffer/IasAudioRingBuffer.cpp \
    ../private/src/audiobuffer/IasAudioRingBufferTypes.cpp \
    ../private/src/audiobuffer/IasAudioRingBufferResult.cpp \
    ../private/src/audiobuffer/IasAudioRingBufferReal.cpp \
    ../private/src/audiobuffer/IasAudioRingBufferMirror.cpp \
    ../private/src/audiobuffer/IasAudioRingBufferFactory.cpp

LOCAL_SRC_FILES += \
    ../private/src/samplerateconverter/IasSrcController.cpp \
    ../private/src/samplerateconverter/IasSrcFarrow.cpp \
    ../private/src/samplerateconverter/IasSrcFarrowFirFilter.cpp \
    ../private/src/samplerateconverter/IasSrcWrapper.cpp

LOCAL_SRC_FILES += \
    ../private/src/helper/IasCopyAudioAreaBuffers.cpp

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../public/inc \
    $(LOCAL_PATH)/../private/inc \
    bionic

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/../public/inc

LOCAL_STATIC_LIBRARIES := \
    libtbb \
    libboost \
    libboost_system \
    libboost_filesystem

LOCAL_SHARED_LIBRARIES := \
    libias-android-pthread \
    libcutils \
    libasound \
    libias-core_libraries-foundation \
    libias-core_libraries-base \
    libsndfile-1.0.27 \
    libdlt \
    liblog

FD_SIGNAL_PATH := /run/smartx/

LOCAL_CFLAGS := $(IAS_COMMON_CFLAGS) \
    -frtti -fexceptions -msse -msse2 -Werror -Wpointer-arith \
    -DFD_SIGNAL_PATH=\"/mnt/eavb/misc$(FD_SIGNAL_PATH)\"

ifeq ("$(shell [ $(ANDROID_VERSION) -eq $(ANDROID_M) ] && echo true)", "true")
BOOST_SHARED_FOLDER := /mnt/eavb/misc/media/
else
BOOST_SHARED_FOLDER := /mnt/eavb/misc/audioserver/
endif

LOCAL_CFLAGS += \
    -DBOOST_INTERPROCESS_SHARED_DIR_PATH=\"$(BOOST_SHARED_FOLDER)\" \
    -DSHM_ROOT_PATH=\"$(BOOST_SHARED_FOLDER)\"

include $(BUILD_SHARED_LIBRARY)
