#
# Copyright (C) 2018 Intel Corporation.All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
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

ANDROID_SUPPORTED_VERSIONS := 6 7 O 8

#------------------------------------------------------------------
# Determining Android version of current build
# first letter may be major version or may be the dessert letter...
#------------------------------------------------------------------
PLATFORM_VERSION_FIRST_LETTER := $(word 1, $(subst ., ,$(PLATFORM_VERSION)))

$(warning "PLATFORM_VERSION $(PLATFORM_VERSION)")
$(warning "PLATFORM_VERSION_FIRST_LETTER $(PLATFORM_VERSION_FIRST_LETTER)")

$(foreach item, $(ANDROID_SUPPORTED_VERSIONS),\
    $(if $(call streq,$(PLATFORM_VERSION_FIRST_LETTER),$(item)),\
        $(eval ANDROID_VERSION := $(ANDROID_VERSION_$(item))),))

$(if $(ANDROID_VERSION),$(warning "ANDROID_VERSION $(ANDROID_VERSION)"),\
    $(error Unsupported Android version))

#######################################################################

LOCAL_PATH := $(call my-dir)

include $(LOCAL_PATH)/AudioCommon.mk

include $(LOCAL_PATH)/SmartXAlsaPlugin.mk

include $(LOCAL_PATH)/Tools.mk

include $(LOCAL_PATH)/LibasoundModuleRateSmartX.mk

#include $(LOCAL_PATH)/TestSupport.mk

#include $(LOCAL_PATH)/test/Android.mk
