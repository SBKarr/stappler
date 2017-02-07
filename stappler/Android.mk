LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include $(LOCAL_PATH)/../build/android/export.mk
LOCAL_MODULE := stappler_static
LOCAL_MODULE_FILENAME := libstappler

LOCAL_SRC_FILES := $(LOCAL_PATH)/../build/android/$(BUILD_TYPE)/$(TARGET_ARCH_ABI)/libstappler.a

LOCAL_EXPORT_C_INCLUDES := $(patsubst ./%,$(SP_PATH)/%,$(STAPPLER_INCLUDES))

LOCAL_WHOLE_STATIC_LIBRARIES := \
	cocos_png_static \
	cocos_jpeg_static \
	cocos_freetype2_static \
	sqlite_static \
	curl_static \
	cairo_static \
	pixman_static \
	hyphen_static \
	libtess2_static \
	cpufeatures

LOCAL_EXPORT_LDLIBS := -lGLESv2 -lEGL -llog -lz -landroid
                       
include $(PREBUILT_STATIC_LIBRARY)

$(call import-module,libs)
$(call import-module,android/cpufeatures)
