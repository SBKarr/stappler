LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := document_material_static
LOCAL_MODULE_FILENAME := libdocument_material
LOCAL_SRC_FILES := \
	$(shell find $(LOCAL_PATH)/src/mmd/common -name *.cpp) \
	$(shell find $(LOCAL_PATH)/src/mmd/processors -name *.cpp) \
	$(shell find $(LOCAL_PATH)/src/mmd/internals -name *.c) \
	$(shell find $(LOCAL_PATH)/src/mmd/internals -name *.cpp) \
	$(shell find $(LOCAL_PATH)/src/mmd/layout -name *.cpp) \
	$(shell find $(LOCAL_PATH)/src/epub -name *.cpp) \
	$(shell find $(LOCAL_PATH)/src/rich_text -name *.cpp)

LOCAL_EXPORT_C_INCLUDES := \
	$(LOCAL_PATH)/src/mmd/common \
	$(LOCAL_PATH)/src/mmd/processors \
	$(LOCAL_PATH)/src/mmd/layout \
	$(LOCAL_PATH)/src/epub \
	$(LOCAL_PATH)/src/rich_text \
	$(LOCAL_PATH)/src/rich_text/articles \
	$(LOCAL_PATH)/src/rich_text/common \
	$(LOCAL_PATH)/src/rich_text/epub \
	$(LOCAL_PATH)/src/rich_text/gallery \
	$(LOCAL_PATH)/src/rich_text/menu

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/src/mmd/common \
	$(LOCAL_PATH)/src/mmd/processors \
	$(LOCAL_PATH)/src/mmd/layout \
	$(LOCAL_PATH)/src/epub \
	$(LOCAL_PATH)/src/rich_text \
	$(LOCAL_PATH)/src/rich_text/articles \
	$(LOCAL_PATH)/src/rich_text/common \
	$(LOCAL_PATH)/src/rich_text/epub \
	$(LOCAL_PATH)/src/rich_text/gallery \
	$(LOCAL_PATH)/src/rich_text/menu

LOCAL_STATIC_LIBRARIES := stappler_static material_static

include $(BUILD_STATIC_LIBRARY)

$(call import-module,stappler)
$(call import-module,libs)
