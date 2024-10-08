LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libexfat_mkfs
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS = -D_FILE_OFFSET_BITS=64
LOCAL_SRC_FILES =  cbm.c fat.c main.c mkexfat.c rootdir.c uct.c uctc.c vbr.c
LOCAL_C_INCLUDES += $(LOCAL_PATH) \
					vendor/zte/exfat/opensource/exfat-master/libexfat \
					vendor/zte/fuse/include
LOCAL_SYSTEM_EXT_MODULE := true
include $(BUILD_STATIC_LIBRARY)
