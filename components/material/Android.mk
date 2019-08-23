LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include $(LOCAL_PATH)/../build/android/export.mk
LOCAL_MODULE := material_static
LOCAL_MODULE_FILENAME := libmaterial

LOCAL_SRC_FILES := $(LOCAL_PATH)/../build/android/$(BUILD_TYPE)/$(TARGET_ARCH_ABI)/libmaterial.a

LOCAL_EXPORT_C_INCLUDES :=  $(patsubst ./%,$(SP_PATH)/%,$(MATERIAL_INCLUDES))

include $(PREBUILT_STATIC_LIBRARY)

$(call import-module,stappler)
$(call import-module,libs)
