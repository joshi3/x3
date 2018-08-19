LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    librilutilsmtk.c \
    mtk_rs.c

LOCAL_SHARED_LIBRARIES := \
    libcutils libutils liblog libratconfig libcrypto

ifneq ($(MTK_NUM_MODEM_PROTOCOL),1)
    LOCAL_CFLAGS += -DMD_PS_COUNT
endif

ifeq ($(MTK_NUM_MODEM_PROTOCOL), 2)
    LOCAL_CFLAGS += -DMD_PS_COUNT_2
endif

ifeq ($(MTK_NUM_MODEM_PROTOCOL), 3)
    LOCAL_CFLAGS += -DMD_PS_COUNT_3
endif

ifeq ($(MTK_NUM_MODEM_PROTOCOL), 4)
    LOCAL_CFLAGS += -DMD_PS_COUNT_4
endif

ifeq ("$(wildcard vendor/mediatek/internal/mtkrild_enable)","")
    LOCAL_CFLAGS += -D__PRODUCTION_RELEASE__
endif

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../include \
    system/netd/include

LOCAL_EXPORT_C_INCLUDE_DIRS := \
    $(LOCAL_PATH)/../include

LOCAL_PROTOC_OPTIMIZE_TYPE := nanopb-c-enable_malloc

LOCAL_MODULE:= librilutilsmtk
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk

include $(BUILD_SHARED_LIBRARY)
