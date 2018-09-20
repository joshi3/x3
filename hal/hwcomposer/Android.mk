LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	hwc2_api.cpp

LOCAL_CFLAGS := \
	-DLOG_TAG=\"hwcomposer\"

LOCAL_CFLAGS += -DMTK_VIRTUAL_SUPPORT
LOCAL_CFLAGS += -DMTK_GMO_RAM_OPTIMIZE


ifeq ($(TARGET_FORCE_HWC_FOR_VIRTUAL_DISPLAYS), true)
LOCAL_CFLAGS += -DMTK_FORCE_HWC_COPY_VDS
endif

ifeq ($(TARGET_RUNNING_WITHOUT_SYNC_FRAMEWORK), true)
LOCAL_CFLAGS += -DMTK_WITHOUT_PRIMARY_PRESENT_FENCE
endif

LOCAL_CFLAGS += -DMTK_CONTROL_POWER_WITH_FRAMEBUFFER_DEVICE
LOCAL_CFLAGS += -DMTK_HWC_VER_2_0
LOCAL_CFLAGS += -USE_HWC2

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include

LOCAL_STATIC_LIBRARIES += \
	hwcomposer.2

LOCAL_SHARED_LIBRARIES := \
	libui \
	libutils \
	libcutils \
        liblog \
	libsync \
	libion \
	libbwc \
	libion_mtk \
	libdpframework \
	libhardware \
	libgralloc_extra \
	libdl \
	libbinder \
	libpower \
	libgui \
	libm4u \
	libged \
	libui_ext \
	libperfservicenative

# HAL module implemenation stored in
# hw/<OVERLAY_HARDWARE_MODULE_ID>.<ro.product.board>.so
LOCAL_MODULE := hwcomposer.mt6795
LOCAL_PROPRIETARY_MODULE := true

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_RELATIVE_PATH := hw
include $(BUILD_SHARED_LIBRARY)
