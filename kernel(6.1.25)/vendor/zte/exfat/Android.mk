LOCAL_PATH := $(call my-dir)
FUSE_EXFAT_ROOT := $(call my-dir)

include $(CLEAR_VARS)

ifeq (PARAGON,$(ZTE_EXFAT_USES))
  include $(LOCAL_PATH)/paragon/Android.mk
endif

ifeq (opensource,$(ZTE_EXFAT_USES))
  include $(FUSE_EXFAT_ROOT)/opensource/exfat-master/Android.mk
endif
