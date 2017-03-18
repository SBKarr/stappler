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
OSTYPE_CFLAGS := -DIOS -Wall -fPIC -DUSE_FILE32API -DCC_TARGET_OS_IPHONE -arch $(IOS_ARCH) -isysroot $(SYSROOT) -miphoneos-version-min=$(MIN_IOS_VERSION)
OSTYPE_CPPFLAGS := -Wno-overloaded-virtual -frtti
OSTYPE_GCHFLAGS := -x c++-header
OSTYPE_COMMON_LIBS :=
OSTYPE_CLI_LIBS :=
OSTYPE_STAPPLER_LIBS :=
OSTYPE_LDFLAGS :=  -miphoneos-version-min=$(MIN_IOS_VERSION)
OSTYPE_EXEC_FLAGS :=

GLOBAL_CPP := $(XCODE_BIN_PATH)/clang++
GLOBAL_CC := $(XCODE_BIN_PATH)/clang
GLOBAL_LIBTOOL := $(XCODE_BIN_PATH)/ar rcs

ifdef RELEASE
TOOLKIT_OUTPUT := $(IOS_ROOT)/$(IOS_ARCH)
GLOBAL_CFLAGS := -O2 -DNDEBUG $(OSTYPE_CFLAGS) $(GLOBAL_CFLAGS)
else
TOOLKIT_OUTPUT := $(IOS_ROOT)/$(IOS_ARCH)
GLOBAL_CFLAGS := -g -DDEBUG -DCOCOS2D_DEBUG=1 $(OSTYPE_CFLAGS) $(GLOBAL_CFLAGS)
endif

OBJC := 1
