# Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>
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
OSTYPE_CFLAGS := -DANDROID -Wall -Wno-overloaded-virtual -fPIC -frtti -DUSE_FILE32API --sysroot $(ANDROID_TOOLCHAIN)/sysroot -I$(NDK)/sources/android/cpufeatures
OSTYPE_COMMON_LIBS := 
OSTYPE_CLI_LIBS := 
OSTYPE_STAPPLER_LIBS := 
OSTYPE_LDFLAGS := 
OSTYPE_EXEC_FLAGS := 

ifeq ($(ANDROID_ARCH),armeabi-v7a)
OSTYPE_CFLAGS += -march=armv7-a -mfloat-abi=softfp -mfpu=neon-vfpv3
endif

ifeq ($(ANDROID_ARCH),x86)
OSTYPE_CFLAGS += -mstackrealign -fstack-protector-strong -mstack-alignment=16
endif

GLOBAL_CPP := $(ANDROID_TOOLCHAIN)/bin/$(ANDROID_TARGET)-clang++
GLOBAL_CC := $(ANDROID_TOOLCHAIN)/bin/$(ANDROID_TARGET)-clang
GLOBAL_AR := $(ANDROID_TOOLCHAIN)/bin/$(ANDROID_TARGET)-ar rcs

ifdef RELEASE
TOOLKIT_OUTPUT := $(TOOLKIT_OUTPUT)/android/release/$(ANDROID_ARCH)
GLOBAL_CFLAGS := -O2 -DNDEBUG $(OSTYPE_CFLAGS) $(GLOBAL_CFLAGS)
else
TOOLKIT_OUTPUT := $(TOOLKIT_OUTPUT)/android/debug/$(ANDROID_ARCH)
GLOBAL_CFLAGS := -g -DDEBUG -DCOCOS2D_DEBUG=1 $(OSTYPE_CFLAGS) $(GLOBAL_CFLAGS)
endif
