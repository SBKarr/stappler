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

OSTYPE_PREBUILT_PATH := libs/ios/$(IOS_ARCH)/lib
OSTYPE_INCLUDE :=  libs/ios/$(IOS_ARCH)/include
OSTYPE_CFLAGS := -DIOS -Wall -fPIC -DUSE_FILE32API -DCC_TARGET_OS_IPHONE -arch $(IOS_ARCH) -isysroot $(SYSROOT) -miphoneos-version-min=$(MIN_IOS_VERSION) -Wno-gnu-string-literal-operator-template
OSTYPE_CPPFLAGS := -Wno-overloaded-virtual -frtti
OSTYPE_GCHFLAGS := -x c++-header

OSTYPE_COMMON_LIBS_LIST := \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libcurl.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libbrotlidec.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libbrotlienc.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libbrotlicommon.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libpng.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libjpeg.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libwebp.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libidn2.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libmbedx509.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libmbedcrypto.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libmbedtls.a

OSTYPE_STAPPLER_LIBS_LIST := \
	$(OSTYPE_COMMON_LIBS_LIST) \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libfreetype.a \

OSTYPE_COMMON_LIBS := $(OSTYPE_COMMON_LIBS_LIST) -lz
OSTYPE_CLI_LIBS :=  $(OSTYPE_COMMON_LIBS)

OSTYPE_STAPPLER_LIBS := $(OSTYPE_STAPPLER_LIBS_LIST) \
	-lz -lsqlite3 -liconv \
	-framework Foundation \
	-framework UIKit \
	-framework OpenGLES \
	-framework Security \
	-framework SystemConfiguration \
	-framework StoreKit \
	-framework CoreGraphics \
	-framework QuartzCore

OSTYPE_LDFLAGS :=  -arch $(IOS_ARCH) -isysroot $(SYSROOT) -miphoneos-version-min=$(MIN_IOS_VERSION) -rdynamic
OSTYPE_EXEC_FLAGS :=

GLOBAL_CPP := $(XCODE_BIN_PATH)/clang++
GLOBAL_CC := $(XCODE_BIN_PATH)/clang
GLOBAL_AR := $(XCODE_BIN_PATH)/ar rcs
GLOBAL_LIBTOOL := $(XCODE_BIN_PATH)/libtool

ifdef RELEASE
TOOLKIT_OUTPUT := $(IOS_ROOT)/$(IOS_ARCH)
GLOBAL_CFLAGS := -Oz -DNDEBUG $(OSTYPE_CFLAGS) $(GLOBAL_CFLAGS)
else
TOOLKIT_OUTPUT := $(IOS_ROOT)/$(IOS_ARCH)
GLOBAL_CFLAGS := -g -DDEBUG -DCOCOS2D_DEBUG=1 $(OSTYPE_CFLAGS) $(GLOBAL_CFLAGS)
endif

BUILD_OBJC := 1
