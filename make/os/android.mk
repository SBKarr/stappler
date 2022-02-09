# Copyright (c) 2017-2022 Roman Katuntsev <sbkarr@stappler.org>
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

ANDROID_TARGET := arm-linux-androideabi
ifeq ($(ANDROID_ARCH),x86)
ANDROID_TARGET := i686-linux-android
endif
ifeq ($(ANDROID_ARCH),arm64-v8a)
ANDROID_TARGET := aarch64-linux-android
endif
ifeq ($(ANDROID_ARCH),x86_64)
ANDROID_TARGET := x86_64-linux-android
endif

ANDROID_TOOLCHAIN := $(GLOBAL_ROOT)/libs/android/toolchains/$(ANDROID_ARCH)

### OS vars

OSTYPE_PREBUILT_PATH := libs/android/$(ANDROID_ARCH)/lib
OSTYPE_INCLUDE :=  libs/android/$(ANDROID_ARCH)/include

OSTYPE_COMMON_LIBS_LIST := \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libgnutls.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libhogweed.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libnettle.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libgmp.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libidn2.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libunistring.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libngtcp2.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libngtcp2_crypto_gnutls.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libnghttp3.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libbrotlidec.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libbrotlienc.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libbrotlicommon.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libpng16.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libgif.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libjpeg.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libwebp.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libbacktrace.a\
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libcurl.a

OSTYPE_STAPPLER_LIBS_LIST := \
	$(OSTYPE_COMMON_LIBS_LIST) \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libsqlite3.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libfreetype.a

OSTYPE_CFLAGS := -DANDROID -Wall -fPIC -DUSE_FILE32API --sysroot $(ANDROID_TOOLCHAIN)/sysroot -I$(NDK)/sources/android/cpufeatures
OSTYPE_CPPFLAGS := -Wno-overloaded-virtual -Wno-gnu-string-literal-operator-template -frtti
OSTYPE_GCHFLAGS := -x c++-header

OSTYPE_COMMON_LIBS := -l:libcurl.a \
	-l:libgnutls.a -l:libhogweed.a -l:libnettle.a -l:libgmp.a -l:libidn2.a -l:libunistring.a \
	-l:libngtcp2.a -l:libngtcp2_crypto_gnutls.a -l:libnghttp3.a \
	-l:libbrotlidec.a -l:libbrotlienc.a -l:libbrotlicommon.a \
	-l:libpng16.a -l:libgif.a -l:libjpeg.a -l:libwebp.a \
	-l:libbacktrace.a -lz -lm -landroid -llog

OSTYPE_CLI_LIBS :=  $(OSTYPE_COMMON_LIBS) -l:libsqlite3.a

OSTYPE_STAPPLER_LIBS := $(OSTYPE_CLI_LIBS) -l:libfreetype.a -lGLESv2 -lEGL

OSTYPE_LDFLAGS := -Wl,-z,defs -rdynamic
OSTYPE_EXEC_FLAGS := 


ifeq ($(ANDROID_ARCH),armeabi-v7a)
OSTYPE_CFLAGS += -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16 # `-mfpu=neon-vfpv3` is better, but causes crush on devices without NEON
endif

ifeq ($(ANDROID_ARCH),x86)
OSTYPE_CFLAGS += -mstackrealign -fstack-protector-strong -mstack-alignment=16
endif

GLOBAL_CPP := $(ANDROID_TOOLCHAIN)/bin/$(ANDROID_TARGET)-clang++
GLOBAL_CC := $(ANDROID_TOOLCHAIN)/bin/$(ANDROID_TARGET)-clang
GLOBAL_AR := $(ANDROID_TOOLCHAIN)/bin/$(ANDROID_TARGET)-ar

LOCAL_OPTIMIZATION ?= -O2
GLOBAL_OPTIMIZATION := $(LOCAL_OPTIMIZATION)

ifdef RELEASE
TOOLKIT_OUTPUT := $(TOOLKIT_OUTPUT)/android/release/$(ANDROID_ARCH)
GLOBAL_CFLAGS := $(GLOBAL_OPTIMIZATION) -DNDEBUG $(OSTYPE_CFLAGS) $(GLOBAL_CFLAGS)
else
TOOLKIT_OUTPUT := $(TOOLKIT_OUTPUT)/android/debug/$(ANDROID_ARCH)
GLOBAL_CFLAGS := -g $(GLOBAL_OPTIMIZATION) -DDEBUG -DCOCOS2D_DEBUG=1 $(OSTYPE_CFLAGS) $(GLOBAL_CFLAGS)
endif
