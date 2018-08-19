# Copyright 2006 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_VENDOR_MODULE := true

LOCAL_SRC_FILES:= \
    ril.cpp \
    ril_event.cpp\
    RilSapSocket.cpp \
    ril_service.cpp \
    sap_service.cpp \
    RilOpProxy.cpp

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libutils \
    libbinder \
    libcutils \
    libhardware_legacy \
    librilproxyutils \
    libhidlbase  \
    libhidltransport \
    libhwbinder \
    librilutilsmtk \

include $(LOCAL_PATH)/hidl.mk

LOCAL_STATIC_LIBRARIES := \
    libprotobuf-c-nano-enable_malloc \

#LOCAL_CFLAGS := -DANDROID_MULTI_SIM -DDSDA_RILD1
LOCAL_CFLAGS += -Wno-unused-parameter
ifneq ($(MTK_NUM_MODEM_PROTOCOL),1)
    LOCAL_CFLAGS += -DANDROID_MULTI_SIM
endif

ifeq ($(MTK_NUM_MODEM_PROTOCOL), 2)
    LOCAL_CFLAGS += -DANDROID_SIM_COUNT_2
endif

ifeq ($(MTK_NUM_MODEM_PROTOCOL), 3)
    LOCAL_CFLAGS += -DANDROID_SIM_COUNT_3
endif

ifeq ($(MTK_NUM_MODEM_PROTOCOL), 4)
    LOCAL_CFLAGS += -DANDROID_SIM_COUNT_4
endif

ifeq ($(HAVE_AEE_FEATURE),yes)
    LOCAL_SHARED_LIBRARIES += libaedv
    LOCAL_CFLAGS += -DHAVE_AEE_FEATURE
endif

ifeq ($(MTK_TC1_FEATURE),yes)
    LOCAL_CFLAGS += -DMTK_TC1_FEATURE
endif

LOCAL_C_INCLUDES += external/nanopb-c \
                    $(TOP)/$(DEVICE_PATH)/hal/ril/binary/inc

LOCAL_CFLAGS += -DMTK_USE_HIDL

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../include
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/../../include

LOCAL_MODULE:= librilproxy
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
LOCAL_CLANG := true
LOCAL_SANITIZE := integer

include $(BUILD_SHARED_LIBRARY)
