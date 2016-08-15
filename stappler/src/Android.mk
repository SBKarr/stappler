LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := stappler_static
LOCAL_MODULE_FILENAME := libstappler

LOCAL_SRC_FILES := \
		$(shell find $(LOCAL_PATH)/actions -name *.cpp) \
		$(shell find $(LOCAL_PATH)/components -name *.cpp) \
		$(shell find $(LOCAL_PATH)/core -name *.cpp) \
		$(shell find $(LOCAL_PATH)/features -name *.cpp) \
		$(shell find $(LOCAL_PATH)/nodes -name *.cpp) \
		$(shell find $(LOCAL_PATH)/platform/android -name *.cpp) \
		$(shell find $(LOCAL_PATH)/platform/universal -name *.cpp) \

LOCAL_EXPORT_C_INCLUDES := \
		$(shell find $(LOCAL_PATH) -type d)

LOCAL_C_INCLUDES := \
		$(shell find $(LOCAL_PATH) -type d)

LOCAL_WHOLE_STATIC_LIBRARIES := \
		cocos2dx_static \
		common_static \
		cocos_freetype_static \
		sqlite_static \
		curl_static \
		crypto_static \
		ssl_static \
		cairo_static \
		pixman_static \
		hyphen_static \

LOCAL_CFLAGS := 
LOCAL_EXPORT_CFLAGS :=

ifdef SP_NOSTOREKIT
LOCAL_CFLAGS +=  -DSP_NOSTOREKIT
LOCAL_EXPORT_CFLAGS +=  -DSP_NOSTOREKIT
endif

include $(BUILD_STATIC_LIBRARY)

$(call import-module,common)
$(call import-module,cocos)
$(call import-module,libs)
