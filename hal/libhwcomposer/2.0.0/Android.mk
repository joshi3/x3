# build hwcomposer static library

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := hwcomposer.2
LOCAL_PROPRIETARY_MODULE := true

LOCAL_MODULE_CLASS := STATIC_LIBRARIES

LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
LOCAL_C_INCLUDES += \
	frameworks/native/services/surfaceflinger \
	$(LOCAL_PATH)/../mt6795 \
	$(LOCAL_PATH)/.. \
	$(LOCAL_PATH)/../include \
	$(TOP)/system/core/libion/include \
	$(TOP)/system/core/libsync/include \
	$(TOP)/system/core/libsync \
	$(TOP)/system/core/include \
	$(TOP)/system/core/base/include \
	frameworks/native/libs/nativewindow/include \
	frameworks/native/libs/nativebase/include \
	frameworks/native/libs/arect/include \

LOCAL_SHARED_LIBRARIES := \
	libui \
	libdpframework \
	libged

LOCAL_SRC_FILES := \
	hwc2.cpp \
	dispatcher.cpp \
	worker.cpp \
	display.cpp \
	hwdev.cpp \
	event.cpp \
	overlay.cpp \
	queue.cpp \
	sync.cpp \
	composer.cpp \
	blitdev.cpp \
	bliter.cpp \
	platform_common.cpp \
	post_processing.cpp \
	../utils/tools.cpp \
	../utils/debug.cpp \
	../utils/transform.cpp \
	../utils/devicenode.cpp \
	color.cpp

LOCAL_SRC_FILES += \
	../mt6795/platform.cpp

LOCAL_CFLAGS:= \
	-DLOG_TAG=\"hwcomposer\"

ifneq ($(strip $(BOARD_VNDK_SUPPORT)),current)
LOCAL_CFLAGS += -DBOARD_VNDK_SUPPORT
endif

#LOCAL_CFLAGS += -DUSE_WDT_IOCTL

LOCAL_CFLAGS += -DUSE_NATIVE_FENCE_SYNC

LOCAL_CFLAGS += -DUSE_SYSTRACE

LOCAL_CFLAGS += -DMTK_HWC_VER_2_0

LOCAL_CFLAGS += -DUSE_HWC2

LOCAL_CFLAGS += -DUSE_SWWATCHDOG

LOCAL_CFLAGS += -DUSES_GRALLOC1


#LOCAL_CFLAGS += -DMTK_HWC_PROFILING


include $(BUILD_STATIC_LIBRARY)
