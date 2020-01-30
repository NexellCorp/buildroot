LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libnx_v4l2

ANDROID_VERSION_STR := $(PLATFORM_VERSION)
ANDROID_VERSION := $(firstword $(ANDROID_VERSION_STR))
ifeq ($(ANDROID_VERSION), 9)
LOCAL_VENDOR_MODULE := true
endif

LOCAL_SRC_FILES := \
	nx-v4l2.c
LOCAL_C_INCLUDES += \
	system/core/include/utils \
	frameworks/native/include \
	$(call include-path-for)

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := libnx_v4l2

LOCAL_SRC_FILES := \
	nx-v4l2.c
LOCAL_C_INCLUDES += \
	system/core/include/utils \
	frameworks/native/include \
	$(call include-path-for)

include $(BUILD_STATIC_LIBRARY)
