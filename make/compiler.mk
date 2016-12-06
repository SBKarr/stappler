# Copyright (c) 2016 Roman Katuntsev <sbkarr@stappler.org>
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

ifndef TOOLKIT_OUTPUT
TOOLKIT_OUTPUT := $(GLOBAL_ROOT)/build
endif


ifdef RELEASE
IOS_ROOT := $(TOOLKIT_OUTPUT)/ios/release
else
IOS_ROOT := $(TOOLKIT_OUTPUT)/ios/debug
endif


ifdef ANDROID_ARCH


NDK ?= $(ANDROID_NDK_ROOT)

ANDROID_TARGET := arm-linux-androideabi
ifeq ($(ANDROID_ARCH),x86)
ANDROID_TARGET := i686-linux-android
endif

ANDROID_TOOLCHAIN := $(GLOBAL_ROOT)/libs/android/toolchains/$(ANDROID_ARCH)

OSTYPE_PREBUILT_PATH := libs/android/$(ANDROID_ARCH)/lib
OSTYPE_INCLUDE :=  libs/android/$(ANDROID_ARCH)/include
OSTYPE_CFLAGS := -DANDROID -Wall -Wno-overloaded-virtual -fPIC -frtti -DUSE_FILE32API --sysroot $(ANDROID_TOOLCHAIN)/sysroot -I$(NDK)/sources/android/cpufeatures
OSTYPE_COMMON_LIBS := 
OSTYPE_CLI_LIBS := 
OSTYPE_STAPPLER_LIBS := 
OSTYPE_LDFLAGS := 
OSTYPE_EXEC_FLAGS := 

ifeq ($(ANDROID_ARCH),armeabi-v7a)
OSTYPE_CFLAGS += -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16
endif

GLOBAL_CPP := $(ANDROID_TOOLCHAIN)/bin/$(ANDROID_TARGET)-clang++
GLOBAL_CC := $(ANDROID_TOOLCHAIN)/bin/$(ANDROID_TARGET)-clang
GLOBAL_AR := $(ANDROID_TOOLCHAIN)/bin/$(ANDROID_TARGET)-ar rcs

ifdef RELEASE
TOOLKIT_OUTPUT := $(TOOLKIT_OUTPUT)/android/release/$(ANDROID_ARCH)
GLOBAL_CFLAGS := -Os -DNDEBUG $(OSTYPE_CFLAGS) $(GLOBAL_CFLAGS)
else
TOOLKIT_OUTPUT := $(TOOLKIT_OUTPUT)/android/debug/$(ANDROID_ARCH)
GLOBAL_CFLAGS := -g -DDEBUG -DCOCOS2D_DEBUG=1 $(OSTYPE_CFLAGS) $(GLOBAL_CFLAGS)
endif


else ifdef IOS_ARCH


XCODE_BIN_PATH := /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin

OSTYPE_PREBUILT_PATH := libs/ios/$(IOS_ARCH)/lib
OSTYPE_INCLUDE :=  libs/ios/$(IOS_ARCH)/include
OSTYPE_CFLAGS := -DIOS -Wall -Wno-overloaded-virtual -fPIC -frtti -DUSE_FILE32API -DCC_TARGET_OS_IPHONE -arch $(IOS_ARCH) -isysroot $(SYSROOT) -miphoneos-version-min=$(MIN_IOS_VERSION)
OSTYPE_COMMON_LIBS :=
OSTYPE_CLI_LIBS :=
OSTYPE_STAPPLER_LIBS :=
OSTYPE_LDFLAGS :=  -miphoneos-version-min=$(MIN_IOS_VERSION)
OSTYPE_EXEC_FLAGS :=

GLOBAL_CPP := $(XCODE_BIN_PATH)/clang++
GLOBAL_CC := $(XCODE_BIN_PATH)/clang
GLOBAL_AR := $(XCODE_BIN_PATH)/libtool -static

ifdef RELEASE
TOOLKIT_OUTPUT := $(IOS_ROOT)/$(IOS_ARCH)
GLOBAL_CFLAGS := -Os -DNDEBUG $(OSTYPE_CFLAGS) $(GLOBAL_CFLAGS)
else
TOOLKIT_OUTPUT := $(IOS_ROOT)/$(IOS_ARCH)
GLOBAL_CFLAGS := -g -DDEBUG -DCOCOS2D_DEBUG=1 $(OSTYPE_CFLAGS) $(GLOBAL_CFLAGS)
endif

OBJC := 1


else # ANDROID_ARCH


ifeq ($(shell uname),Darwin)
OSTYPE_PREBUILT_PATH := libs/mac/x86_64/lib
OSTYPE_INCLUDE :=  libs/mac/x86_64/include
OSTYPE_CFLAGS := -DMACOSX  -DUSE_FILE32API -Wall -Wno-overloaded-virtual -fPIC -frtti
OSTYPE_COMMON_LIBS := -lpthread \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libcurl.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libpng.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libjpeg.a \
	-framework Foundation -lz -framework Security

OSTYPE_CLI_LIBS += $(OSTYPE_COMMON_LIBS) \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libsqlite3.a \
	-ldl -framework SystemConfiguration

OSTYPE_STAPPLER_LIBS += $(OSTYPE_CLI_LIBS) \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libhyphen.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libglfw3.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libcairo.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libpixman-1.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libfreetype.a \
	-framework OpenGL -framework CoreGraphics -framework Cocoa -framework IOKit -framework CoreVideo

OSTYPE_LDFLAGS :=
OSTYPE_EXEC_FLAGS :=
CLANG := 1
OBJC := 1
else ifeq ($(shell uname -o),Cygwin)
OSTYPE_PREBUILT_PATH := libs/win32/x86_64/lib
OSTYPE_INCLUDE := libs/win32/x86_64/include
OSTYPE_CFLAGS := -DCYGWIN -Wall -Wno-overloaded-virtual -frtti -D_WIN32 -DGLEW_STATIC
OSTYPE_COMMON_LIBS := -static -lpng -ljpeg -lcurl -lmbedx509 -lmbedtls -lmbedcrypto -lz -lws2_32
OSTYPE_CLI_LIBS += $(OSTYPE_COMMON_LIBS) -lsqlite3
OSTYPE_STAPPLER_LIBS += $(OSTYPE_CLI_LIBS) -lsqlite3 -lglfw3 -lglew32 -lopengl32 -lGdi32 -lcairo -lpixman-1 -lfreetype
OSTYPE_LDFLAGS := -municode
OSTYPE_EXEC_FLAGS := -static-libgcc -static-libstdc++
else
OSTYPE_PREBUILT_PATH := libs/linux/x86_64/lib
OSTYPE_INCLUDE :=  libs/linux/x86_64/include
OSTYPE_CFLAGS := -DLINUX -Wall -Wno-overloaded-virtual -fPIC -frtti
OSTYPE_COMMON_LIBS := -lpthread -l:libcurl.a -l:libmbedtls.a -l:libmbedx509.a -l:libmbedcrypto.a -l:libpng.a -l:libjpeg.a -lz
OSTYPE_CLI_LIBS += $(OSTYPE_COMMON_LIBS) -l:libsqlite3.a -ldl
OSTYPE_STAPPLER_LIBS += $(OSTYPE_CLI_LIBS) -l:libhyphen.a -l:libglfw3.a -l:libcairo.a -l:libpixman-1.a -l:libfreetype.a -lGLEW -lGL -lXxf86vm -lX11 -lXrandr -lXi -lXinerama -lXcursor
OSTYPE_LDFLAGS := -Wl,-z,defs -rdynamic
OSTYPE_EXEC_FLAGS :=
endif

ifndef GLOBAL_CPP
ifdef CLANG
ifeq ($(CLANG),1)
GLOBAL_CPP := clang++
else
GLOBAL_CPP := clang++-$(CLANG)
endif
else
ifdef MINGW
GLOBAL_CPP := $(MINGW)-g++
else
GLOBAL_CPP := g++
endif
endif
endif

ifndef GLOBAL_CC
ifdef CLANG
ifeq ($(CLANG),1)
GLOBAL_CC := clang
else
GLOBAL_CC := clang-$(CLANG)
endif
else
ifdef MINGW
GLOBAL_CC := $(MINGW)-gcc
else
GLOBAL_CC := gcc
endif
endif
endif

ifdef RELEASE
TOOLKIT_OUTPUT := $(TOOLKIT_OUTPUT)/$(notdir $(GLOBAL_CC))/release
ifeq ($(LOCAL_TOOLKIT),serenity)
GLOBAL_CFLAGS := -O2 -g -DNDEBUG $(OSTYPE_CFLAGS) $(GLOBAL_CFLAGS)
else
GLOBAL_CFLAGS := -Os -DNDEBUG $(OSTYPE_CFLAGS) $(GLOBAL_CFLAGS)
endif
else
TOOLKIT_OUTPUT := $(TOOLKIT_OUTPUT)/$(notdir $(GLOBAL_CC))/debug
GLOBAL_CFLAGS := -g -DDEBUG -DCOCOS2D_DEBUG=1 $(OSTYPE_CFLAGS) $(GLOBAL_CFLAGS)
endif


endif # ANDROID_ARCH
