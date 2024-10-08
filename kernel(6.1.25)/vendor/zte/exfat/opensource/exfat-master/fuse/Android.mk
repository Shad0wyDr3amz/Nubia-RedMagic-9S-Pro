LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := libexfat_mount
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS = -D_FILE_OFFSET_BITS=64
LOCAL_SRC_FILES = main.c 
LOCAL_C_INCLUDES += $(LOCAL_PATH) \
					vendor/zte/exfat/opensource/exfat-master/libexfat \
					vendor/zte/fuse/include \
					vendor/zte/fuse/android
LOCAL_SYSTEM_EXT_MODULE := true
include $(BUILD_STATIC_LIBRARY)
