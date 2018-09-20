LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libui_ext
LOCAL_PROPRIETARY_MODULE := true

LOCAL_MODULE_TAGS := optional

ifneq ($(strip $(TARGET_BUILD_VARIANT)), eng)
LOCAL_CFLAGS += -DMTK_USER_BUILD
endif

LOCAL_SRC_FILES := \
	FpsCounter.cpp \
	SWWatchDog.cpp

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	external/zlib \

LOCAL_SHARED_LIBRARIES := \
	libutils \
	libcutils \
	liblog \
	libhardware \

	LOCAL_CFLAGS += -DMTK_AOSP_ENHANCEMENT
	LOCAL_CPPFLAGS += -DMTK_AOSP_ENHANCEMENT

include $(BUILD_SHARED_LIBRARY)
