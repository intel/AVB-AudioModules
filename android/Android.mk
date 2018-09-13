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

#---------------------------------------------------------------------------------------------------
# Android Version Constants definition:
# Redefine here android version in order to be able to use equality, greater than, etc.. rules.
# Use
#   ANDROID_VERSION_<Major or Dessert Letter> to provide the adaption
#
# You can use the conditional expressions rules as:
# ifeq ("$(shell [ $(ANDROID_VERSION) -gt $(ANDROID_N) ] && echo true)", "true")
#---------------------------------------------------------------------------------------------------
ANDROID_M := 6
ANDROID_N := 7
ANDROID_O := 8

ANDROID_VERSION_6 := $(ANDROID_M)
ANDROID_VERSION_7 := $(ANDROID_N)
ANDROID_VERSION_8 := $(ANDROID_O)
ANDROID_VERSION_O := $(ANDROID_O)
ANDROID_VERSION_OMR1 := $(ANDROID_O)

ANDROID_SUPPORTED_VERSIONS := 6 7 O 8 OMR1

#------------------------------------------------------------------
# Determining Android version of current build
# first letter may be major version or may be the dessert letter...
#------------------------------------------------------------------
PLATFORM_VERSION_FIRST_LETTER := $(word 1, $(subst ., ,$(PLATFORM_VERSION)))

$(info "PLATFORM_VERSION $(PLATFORM_VERSION)")
$(info "PLATFORM_VERSION_FIRST_LETTER $(PLATFORM_VERSION_FIRST_LETTER)")

$(foreach item, $(ANDROID_SUPPORTED_VERSIONS),\
    $(if $(call streq,$(PLATFORM_VERSION_FIRST_LETTER),$(item)),\
        $(eval ANDROID_VERSION := $(ANDROID_VERSION_$(item))),))

$(if $(ANDROID_VERSION),$(info "ANDROID_VERSION $(ANDROID_VERSION)"),\
    $(error Unsupported Android version))

#######################################################################

LOCAL_PATH := $(call my-dir)

include $(LOCAL_PATH)/AudioCommon.mk

include $(LOCAL_PATH)/SmartXAlsaPlugin.mk

include $(LOCAL_PATH)/Tools.mk

include $(LOCAL_PATH)/LibasoundModuleRateSmartX.mk

#include $(LOCAL_PATH)/TestSupport.mk

#include $(LOCAL_PATH)/test/Android.mk
