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

THIS_FILE := $(lastword $(MAKEFILE_LIST))

GLOBAL_ROOT := .

DEF_SYSROOT_SIM := "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk"
DEF_SYSROOT_OS := "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk"

DEF_MIN_IOS_VERSION := 8.0

pvs.log:
	pvs-studio-analyzer analyze -o pvs.log -j4

all: libmaterial libstappler libcli libspmin

pvs-analyzer-log: pvs.log
	plog-converter -t errorfile pvs.log

static: libmaterial_static libstappler_static

android:
	$(MAKE) -f $(THIS_FILE) ANDROID_ARCH=armeabi-v7a static
	$(MAKE) -f $(THIS_FILE) ANDROID_ARCH=armeabi static
	$(MAKE) -f $(THIS_FILE) ANDROID_ARCH=x86 static

android-clean:
	$(MAKE) -f $(THIS_FILE) ANDROID_ARCH=armeabi-v7a clean
	$(MAKE) -f $(THIS_FILE) ANDROID_ARCH=armeabi clean
	$(MAKE) -f $(THIS_FILE) ANDROID_ARCH=x86 clean

.PHONY: all analyse-log android

include make/build.mk

ios: $(IOS_ROOT)/libstappler.a $(IOS_ROOT)/libmaterial.a

$(IOS_ROOT)/libstappler.a:
	$(MAKE) -f $(THIS_FILE) IOS_ARCH=armv7 MIN_IOS_VERSION=$(DEF_MIN_IOS_VERSION) SYSROOT=$(DEF_SYSROOT_OS) libstappler_static
	$(MAKE) -f $(THIS_FILE) IOS_ARCH=armv7s MIN_IOS_VERSION=$(DEF_MIN_IOS_VERSION) SYSROOT=$(DEF_SYSROOT_OS) libstappler_static
	$(MAKE) -f $(THIS_FILE) IOS_ARCH=arm64 MIN_IOS_VERSION=$(DEF_MIN_IOS_VERSION) SYSROOT=$(DEF_SYSROOT_OS) libstappler_static
	$(MAKE) -f $(THIS_FILE) IOS_ARCH=i386 MIN_IOS_VERSION=$(DEF_MIN_IOS_VERSION) SYSROOT=$(DEF_SYSROOT_SIM) libstappler_static
	$(MAKE) -f $(THIS_FILE) IOS_ARCH=x86_64 MIN_IOS_VERSION=$(DEF_MIN_IOS_VERSION) SYSROOT=$(DEF_SYSROOT_SIM) libstappler_static
	lipo -create -output $(IOS_ROOT)/libstappler.a $(IOS_ROOT)/armv7/libstappler.a $(IOS_ROOT)/armv7s/libstappler.a $(IOS_ROOT)/arm64/libstappler.a $(IOS_ROOT)/i386/libstappler.a $(IOS_ROOT)/x86_64/libstappler.a

$(IOS_ROOT)/libmaterial.a:
	$(MAKE) -f $(THIS_FILE) IOS_ARCH=armv7 MIN_IOS_VERSION=$(DEF_MIN_IOS_VERSION) SYSROOT=$(DEF_SYSROOT_OS) libmaterial_static
	$(MAKE) -f $(THIS_FILE) IOS_ARCH=armv7s MIN_IOS_VERSION=$(DEF_MIN_IOS_VERSION) SYSROOT=$(DEF_SYSROOT_OS) libmaterial_static
	$(MAKE) -f $(THIS_FILE) IOS_ARCH=arm64 MIN_IOS_VERSION=$(DEF_MIN_IOS_VERSION) SYSROOT=$(DEF_SYSROOT_OS) libmaterial_static
	$(MAKE) -f $(THIS_FILE) IOS_ARCH=i386 MIN_IOS_VERSION=$(DEF_MIN_IOS_VERSION) SYSROOT=$(DEF_SYSROOT_SIM) libmaterial_static
	$(MAKE) -f $(THIS_FILE) IOS_ARCH=x86_64 MIN_IOS_VERSION=$(DEF_MIN_IOS_VERSION) SYSROOT=$(DEF_SYSROOT_SIM) libmaterial_static
	lipo -create -output $(IOS_ROOT)/libmaterial.a $(IOS_ROOT)/armv7/libmaterial.a $(IOS_ROOT)/armv7s/libmaterial.a $(IOS_ROOT)/arm64/libmaterial.a $(IOS_ROOT)/i386/libmaterial.a $(IOS_ROOT)/x86_64/libmaterial.a

.PHONY: ios
