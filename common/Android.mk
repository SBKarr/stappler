LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := common_static
LOCAL_MODULE_FILENAME := libcommon

LOCAL_SRC_FILES := \
		$(shell find $(LOCAL_PATH)/core -name *.cpp) \
		$(shell find $(LOCAL_PATH)/data -name *.cpp) \
		$(shell find $(LOCAL_PATH)/stream -name *.cpp) \
		$(shell find $(LOCAL_PATH)/string -name *.cpp) \
		$(shell find $(LOCAL_PATH)/threads -name *.cpp) \
		$(shell find $(LOCAL_PATH)/utils -name *.cpp) \
		$(shell find $(LOCAL_PATH)/io -name *.cpp) \

LOCAL_EXPORT_C_INCLUDES := \
		$(shell find $(LOCAL_PATH) -type d) \

LOCAL_C_INCLUDES := \
		$(shell find $(LOCAL_PATH) -type d) \

LOCAL_WHOLE_STATIC_LIBRARIES := curl_static \
								crypto_static \
								ssl_static \
								cocos_png_static \

LOCAL_CFLAGS := 
LOCAL_EXPORT_CFLAGS :=

include $(BUILD_STATIC_LIBRARY)

$(call import-module,cocos)
$(call import-module,libs)
