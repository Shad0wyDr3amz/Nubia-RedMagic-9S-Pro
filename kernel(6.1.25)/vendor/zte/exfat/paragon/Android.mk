LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
ifeq ($(TARGET_BOARD_PLATFORM), msm8996)
include $(LOCAL_PATH)/msm8996/Android.mk
endif
ifeq ($(TARGET_BOARD_PLATFORM), msm8937)
include $(LOCAL_PATH)/msm8937/Android.mk
endif
ifeq ($(TARGET_DEVICE), sdm660_64)
include $(LOCAL_PATH)/sdm660_64/Android.mk
endif

