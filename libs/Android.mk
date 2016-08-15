LOCAL_PATH := $(call my-dir)


### JPEG
include $(CLEAR_VARS)
LOCAL_MODULE := cocos_jpeg_static
LOCAL_MODULE_FILENAME := jpeg
LOCAL_SRC_FILES := $(LOCAL_PATH)/android/$(TARGET_ARCH_ABI)/lib/libjpeg.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/android/$(TARGET_ARCH_ABI)/include
include $(PREBUILT_STATIC_LIBRARY)


### PNG
include $(CLEAR_VARS)
LOCAL_MODULE := cocos_png_static
LOCAL_MODULE_FILENAME := png
LOCAL_SRC_FILES := $(LOCAL_PATH)/android/$(TARGET_ARCH_ABI)/lib/libpng.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/android/$(TARGET_ARCH_ABI)/include
include $(PREBUILT_STATIC_LIBRARY)


### Freetype2
include $(CLEAR_VARS)
LOCAL_MODULE := cocos_freetype2_static
LOCAL_MODULE_FILENAME := freetype2
LOCAL_SRC_FILES := $(LOCAL_PATH)/android/$(TARGET_ARCH_ABI)/lib/libfreetype.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/android/$(TARGET_ARCH_ABI)/include
include $(PREBUILT_STATIC_LIBRARY)


### SSL
include $(CLEAR_VARS)
LOCAL_MODULE := mbedtls_static
LOCAL_MODULE_FILENAME := mbedtls
LOCAL_SRC_FILES := $(LOCAL_PATH)/android/$(TARGET_ARCH_ABI)/lib/libmbedtls.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := mbedcrypto_static
LOCAL_MODULE_FILENAME := mbedtls
LOCAL_SRC_FILES := $(LOCAL_PATH)/android/$(TARGET_ARCH_ABI)/lib/libmbedcrypto.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := mbedx509_static
LOCAL_MODULE_FILENAME := mbedtls
LOCAL_SRC_FILES := $(LOCAL_PATH)/android/$(TARGET_ARCH_ABI)/lib/libmbedx509.a
include $(PREBUILT_STATIC_LIBRARY)


### Curl
include $(CLEAR_VARS)
LOCAL_MODULE := curl_static
LOCAL_MODULE_FILENAME := curl
LOCAL_SRC_FILES := $(LOCAL_PATH)/android/$(TARGET_ARCH_ABI)/lib/libcurl.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/android/$(TARGET_ARCH_ABI)/include/curl
LOCAL_WHOLE_STATIC_LIBRARIES := mbedtls_static mbedcrypto_static mbedx509_static
include $(PREBUILT_STATIC_LIBRARY)


### SQLite
include $(CLEAR_VARS)
LOCAL_MODULE := sqlite_static
LOCAL_MODULE_FILENAME := libsqlite
LOCAL_SRC_FILES := $(LOCAL_PATH)/android/$(TARGET_ARCH_ABI)/lib/libsqlite3.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/android/$(TARGET_ARCH_ABI)/include
include $(PREBUILT_STATIC_LIBRARY)


### Pixman
include $(CLEAR_VARS)
LOCAL_MODULE := pixman_static
LOCAL_MODULE_FILENAME := pixman
LOCAL_SRC_FILES := $(LOCAL_PATH)/android/$(TARGET_ARCH_ABI)/lib/libpixman-1.a
include $(PREBUILT_STATIC_LIBRARY)


### Cairo
include $(CLEAR_VARS)
LOCAL_MODULE := cairo_static
LOCAL_MODULE_FILENAME := cairo
LOCAL_SRC_FILES := $(LOCAL_PATH)/android/$(TARGET_ARCH_ABI)/lib/libcairo.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/android/$(TARGET_ARCH_ABI)/include/cairo
LOCAL_WHOLE_STATIC_LIBRARIES := pixman_static
include $(PREBUILT_STATIC_LIBRARY)


### Hyphen
include $(CLEAR_VARS)
LOCAL_MODULE := hyphen_static
LOCAL_MODULE_FILENAME := libhyphen
LOCAL_SRC_FILES := $(LOCAL_PATH)/android/$(TARGET_ARCH_ABI)/lib/libhyphen.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/android/$(TARGET_ARCH_ABI)/include
include $(PREBUILT_STATIC_LIBRARY)

