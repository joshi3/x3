LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
  ratconfig.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include

LOCAL_SHARED_LIBRARIES:= \
  libutils liblog libcutils

LOCAL_MODULE:= libratconfig
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk

LOCAL_EXPORT_C_INCLUDE_DIRS := \
  $(LOCAL_PATH)/include

include $(BUILD_SHARED_LIBRARY)
