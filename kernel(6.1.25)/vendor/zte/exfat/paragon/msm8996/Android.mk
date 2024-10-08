#/*
# * Portions copyright 2009-2014 Paragon Software GmbH
# * $Author: nefedov $
# * Support: oem@paragon-software.com
# * License: This code is licensed under the associated license agreement.
# *
# * ----- Disclaimer ------
# * Paragon Software accepts no responsibility for malfunctions or damage caused
# * by modification of the software or the use of the software in conjunction with
# * hardware configurations, platforms or operating systems other than the
# * recommended or intended hardware configuration, platform or operating system.
# *
# * Copyright (C) 2008 The Android Open Source Project
# * Copyright (C) 2014 Paragon Software GmbH
# *
# * Licensed under the Apache License, Version 2.0 (the "License");
# * you may not use this file except in compliance with the License.
# * You may obtain a copy of the License at
# *
# *      http://www.apache.org/licenses/LICENSE-2.0
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# *
# * NOTE: This file has been modified by Paragon Software Group GmbH.
# * Modifications are licensed under the License.
# */

LOCAL_PATH := $(call my-dir)

COMMON_DLKM_PATH := $(TOP)/device/qcom/common/dlkm

#
# ATTENTION:
#
# The blob file (e.g. libufsd_arm64.bin) to be named <filename>_shipped
# (e.g. libufsd_arm64.bin_shipped) is required.
#
# See section '3.3 Binary Blobs' in kernel/Documentation/kbuild/modules.txt
# as reference.
#

$(warning ****$(TARGET_BOARD_SUFFIX)=TARGET_BOARD_SUFFIX********)
$(shell cp -f $(LOCAL_PATH)/ifslinux/objfre/libufsd_arm64.bin \
 $(LOCAL_PATH)/ifslinux/objfre/libufsd_arm64.bin_shipped)


include $(CLEAR_VARS)
LOCAL_MODULE      := ufsd.ko
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT_SBIN)
include $(COMMON_DLKM_PATH)/AndroidKernelModule.mk

include $(CLEAR_VARS)
LOCAL_MODULE      := jnl.ko
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT_SBIN)
include $(COMMON_DLKM_PATH)/AndroidKernelModule.mk

include $(CLEAR_VARS)
LOCAL_MODULE := chkufsd
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_CERTIFICATE := platform
LOCAL_MODULE_PATH := $(TARGET_OUT)/xbin
LOCAL_SRC_FILES := chkufsd
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := mkexfat
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_CERTIFICATE := platform
LOCAL_MODULE_PATH := $(TARGET_OUT)/xbin
LOCAL_SRC_FILES := mkexfat
include $(BUILD_PREBUILT)
