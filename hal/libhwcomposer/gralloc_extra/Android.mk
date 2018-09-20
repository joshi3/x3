LOCAL_PATH := $(call my-dir)

###############################################################################
# Build Gralloc Extra Vendor library
# #
#
include $(CLEAR_VARS)

LOCAL_MODULE := libgralloc_extra

LOCAL_SRC_FILES := \
    ge_operator.cpp \
    ge_misc.c \
    platform/ge_platform_default.c

# add platform.c
LOCAL_SRC_FILES += platform/mt6795/platform.c

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/include \
    $(TOP)/system/core/libion/include

LOCAL_EXPORT_C_INCLUDE_DIRS := \
    $(LOCAL_PATH)/include

LOCAL_SHARED_LIBRARIES := \
    libhardware \
    libcutils \
    libutils \
    liblog \
    libion \
    libged

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
