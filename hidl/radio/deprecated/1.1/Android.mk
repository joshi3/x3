# This file is autogenerated by hidl-gen. Do not edit manually.

LOCAL_PATH := $(call my-dir)

################################################################################

include $(CLEAR_VARS)
LOCAL_MODULE := vendor.mediatek.hardware.radio.deprecated-V1.1-java
LOCAL_MODULE_CLASS := JAVA_LIBRARIES

intermediates := $(call local-generated-sources-dir, COMMON)

HIDL := $(HOST_OUT_EXECUTABLES)/hidl-gen$(HOST_EXECUTABLE_SUFFIX)

LOCAL_JAVA_LIBRARIES := \
    android.hardware.radio.deprecated-V1.0-java \
    android.hardware.radio-V1.0-java \
    android.hidl.base-V1.0-java \


#
# Build IOemHook.hal
#
GEN := $(intermediates)/device/leeco/x3/hidl/radio/deprecated/V1_1/IOemHook.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/IOemHook.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Ljava \
        -randroid.hardware:hardware/interfaces \
        -randroid.hidl:system/libhidl/transport \
        -rvendor.mediatek.hardware:device/leeco/x3/hidl \
        vendor.mediatek.hardware.radio.deprecated@1.1::IOemHook

$(GEN): $(LOCAL_PATH)/IOemHook.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build IOemHookIndication.hal
#
GEN := $(intermediates)/device/leeco/x3/hidl/radio/deprecated/V1_1/IOemHookIndication.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/IOemHookIndication.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Ljava \
        -randroid.hardware:hardware/interfaces \
        -randroid.hidl:system/libhidl/transport \
        -rvendor.mediatek.hardware:device/leeco/x3/hidl \
        vendor.mediatek.hardware.radio.deprecated@1.1::IOemHookIndication

$(GEN): $(LOCAL_PATH)/IOemHookIndication.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build IOemHookResponse.hal
#
GEN := $(intermediates)/device/leeco/x3/hidl/radio/deprecated/V1_1/IOemHookResponse.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/IOemHookResponse.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Ljava \
        -randroid.hardware:hardware/interfaces \
        -randroid.hidl:system/libhidl/transport \
        -rvendor.mediatek.hardware:device/leeco/x3/hidl \
        vendor.mediatek.hardware.radio.deprecated@1.1::IOemHookResponse

$(GEN): $(LOCAL_PATH)/IOemHookResponse.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)
include $(BUILD_JAVA_LIBRARY)


################################################################################

include $(CLEAR_VARS)
LOCAL_MODULE := vendor.mediatek.hardware.radio.deprecated-V1.1-java-static
LOCAL_MODULE_CLASS := JAVA_LIBRARIES

intermediates := $(call local-generated-sources-dir, COMMON)

HIDL := $(HOST_OUT_EXECUTABLES)/hidl-gen$(HOST_EXECUTABLE_SUFFIX)

LOCAL_STATIC_JAVA_LIBRARIES := \
    android.hardware.radio.deprecated-V1.0-java-static \
    android.hardware.radio-V1.0-java-static \
    android.hidl.base-V1.0-java-static \


#
# Build IOemHook.hal
#
GEN := $(intermediates)/device/leeco/x3/hidl/radio/deprecated/V1_1/IOemHook.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/IOemHook.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Ljava \
        -randroid.hardware:hardware/interfaces \
        -randroid.hidl:system/libhidl/transport \
        -rvendor.mediatek.hardware:device/leeco/x3/hidl \
        vendor.mediatek.hardware.radio.deprecated@1.1::IOemHook

$(GEN): $(LOCAL_PATH)/IOemHook.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build IOemHookIndication.hal
#
GEN := $(intermediates)/device/leeco/x3/hidl/radio/deprecated/V1_1/IOemHookIndication.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/IOemHookIndication.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Ljava \
        -randroid.hardware:hardware/interfaces \
        -randroid.hidl:system/libhidl/transport \
        -rvendor.mediatek.hardware:device/leeco/x3/hidl \
        vendor.mediatek.hardware.radio.deprecated@1.1::IOemHookIndication

$(GEN): $(LOCAL_PATH)/IOemHookIndication.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)

#
# Build IOemHookResponse.hal
#
GEN := $(intermediates)/device/leeco/x3/hidl/radio/deprecated/V1_1/IOemHookResponse.java
$(GEN): $(HIDL)
$(GEN): PRIVATE_HIDL := $(HIDL)
$(GEN): PRIVATE_DEPS := $(LOCAL_PATH)/IOemHookResponse.hal
$(GEN): PRIVATE_OUTPUT_DIR := $(intermediates)
$(GEN): PRIVATE_CUSTOM_TOOL = \
        $(PRIVATE_HIDL) -o $(PRIVATE_OUTPUT_DIR) \
        -Ljava \
        -randroid.hardware:hardware/interfaces \
        -randroid.hidl:system/libhidl/transport \
        -rvendor.mediatek.hardware:device/leeco/x3/hidl \
        vendor.mediatek.hardware.radio.deprecated@1.1::IOemHookResponse

$(GEN): $(LOCAL_PATH)/IOemHookResponse.hal
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(GEN)
include $(BUILD_STATIC_JAVA_LIBRARY)



include $(call all-makefiles-under,$(LOCAL_PATH))
