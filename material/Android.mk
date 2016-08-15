LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := material_static
LOCAL_MODULE_FILENAME := material
LOCAL_SRC_FILES := \
		$(shell find $(LOCAL_PATH) -name "*.cpp")

LOCAL_EXPORT_C_INCLUDES := $(shell find $(LOCAL_PATH) -type d)

LOCAL_C_INCLUDES := $(shell find $(LOCAL_PATH) -type d)

LOCAL_STATIC_LIBRARIES := stappler_static

include $(BUILD_STATIC_LIBRARY)

$(call import-module,stappler/src)
$(call import-module,libs)
