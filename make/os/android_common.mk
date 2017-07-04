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

# Usually we use NDK variable, transferred to make
# (like: `make android NDK=/path/to/ndk`)
# You can use ANDROID_NDK_ROOT env var
NDK ?= $(ANDROID_NDK_ROOT)

# Android API level for old archs (arm, armv7, x86)
ANDROID_API_LOW := 14

# Android API level for modern archs (armv64, x86_64)
ANDROID_API_HIGH := 21

ANDROID_TOOLCHAINS_PATH := $(GLOBAL_ROOT)/libs/android/toolchains

ANDROID_EXPORT_PREFIX ?= $(GLOBAL_ROOT)

### Common
ANDROID_TOOLCHAINS := \
	$(ANDROID_TOOLCHAINS_PATH)/armeabi \
	$(ANDROID_TOOLCHAINS_PATH)/armeabi-v7a \
	$(ANDROID_TOOLCHAINS_PATH)/x86 \
	$(ANDROID_TOOLCHAINS_PATH)/x86_64 \
	$(ANDROID_TOOLCHAINS_PATH)/arm64-v8a \

ANDROID_EXPORT_MK := $(TOOLKIT_OUTPUT)/android/export.mk


$(ANDROID_TOOLCHAINS_PATH)/armeabi:
	@echo "=== armeabi: Prepare android toolchain ==="
	@$(GLOBAL_MKDIR) $(ANDROID_TOOLCHAINS_PATH)
	$(NDK)/build/tools/make_standalone_toolchain.py --arch=arm --api $(ANDROID_API_LOW) --stl=libc++ --install-dir=$(ANDROID_TOOLCHAINS_PATH)/armeabi

$(ANDROID_TOOLCHAINS_PATH)/armeabi-v7a:
	@echo "=== armeabi-v7a: Prepare android toolchain ==="
	@$(GLOBAL_MKDIR) $(ANDROID_TOOLCHAINS_PATH)
	$(NDK)/build/tools/make_standalone_toolchain.py --arch=arm --api $(ANDROID_API_LOW) --stl=libc++ --install-dir=$(ANDROID_TOOLCHAINS_PATH)/armeabi-v7a

$(ANDROID_TOOLCHAINS_PATH)/x86:
	@echo "=== x86: Prepare android toolchain ==="
	@$(GLOBAL_MKDIR) $(ANDROID_TOOLCHAINS_PATH)
	$(NDK)/build/tools/make_standalone_toolchain.py --arch=x86 --api $(ANDROID_API_LOW) --stl=libc++ --install-dir=$(ANDROID_TOOLCHAINS_PATH)/x86

$(ANDROID_TOOLCHAINS_PATH)/x86_64:
	@echo "=== x86_64: Prepare android toolchain ==="
	@$(GLOBAL_MKDIR) $(ANDROID_TOOLCHAINS_PATH)
	$(NDK)/build/tools/make_standalone_toolchain.py --arch=x86_64 --api $(ANDROID_API_HIGH) --stl=libc++ --install-dir=$(ANDROID_TOOLCHAINS_PATH)/x86_64

$(ANDROID_TOOLCHAINS_PATH)/arm64-v8a:
	@echo "=== arm64-v8a: Prepare android toolchain ==="
	@$(GLOBAL_MKDIR) $(ANDROID_TOOLCHAINS_PATH)
	$(NDK)/build/tools/make_standalone_toolchain.py --arch=arm64 --api $(ANDROID_API_HIGH) --stl=libc++ --install-dir=$(ANDROID_TOOLCHAINS_PATH)/arm64-v8a


$(GLOBAL_ROOT)/build/android/export.mk:


android: $(ANDROID_TOOLCHAINS)
	@echo "=== Export android build info ==="
	@$(MAKE) -f $(THIS_FILE) ANDROID_ARCH=armeabi-v7a android-export
	@echo "=== armeabi-v7a: Build static library ==="
	@$(MAKE) -f $(THIS_FILE) ANDROID_ARCH=armeabi-v7a static
	@echo "=== armeabi: Build static library ==="
	@$(MAKE) -f $(THIS_FILE) ANDROID_ARCH=armeabi static
	@echo "=== arm64-v8a: Build static library ==="
	@$(MAKE) -f $(THIS_FILE) ANDROID_ARCH=arm64-v8a static
	@echo "=== x86: Build static library ==="
	@$(MAKE) -f $(THIS_FILE) ANDROID_ARCH=x86 static
	@echo "=== x86_64: Build static library ==="
	@$(MAKE) -f $(THIS_FILE) ANDROID_ARCH=x86_64 static


android-arm: $(ANDROID_TOOLCHAINS_PATH)/armeabi
	@echo "=== armeabi: Build static library ==="
	@$(MAKE) -f $(THIS_FILE) ANDROID_ARCH=armeabi static android-export


android-armv7: $(ANDROID_TOOLCHAINS_PATH)/armeabi-v7a
	@echo "=== armeabi-v7a: Build static library ==="
	@$(MAKE) -f $(THIS_FILE) ANDROID_ARCH=armeabi-v7a static android-export


android-arm64: $(ANDROID_TOOLCHAINS_PATH)/arm64-v8a
	@echo "=== arm64-v8a: Build static library ==="
	@$(MAKE) -f $(THIS_FILE) ANDROID_ARCH=arm64-v8a static android-export


android-x86: $(ANDROID_TOOLCHAINS_PATH)/x86
	@echo "=== x86: Build static library ==="
	@$(MAKE) -f $(THIS_FILE) ANDROID_ARCH=x86 static android-export


android-x86_64: $(ANDROID_TOOLCHAINS_PATH)/x86_64
	@echo "=== x86_64: Build static library ==="
	@$(MAKE) -f $(THIS_FILE) ANDROID_ARCH=x86_64 static android-export


android-clean: $(ANDROID_TOOLCHAINS)
	@echo "=== armeabi-v7a: clean ==="
	@$(MAKE) -f $(THIS_FILE) ANDROID_ARCH=armeabi-v7a clean
	@echo "=== armeabi: clean ==="
	@$(MAKE) -f $(THIS_FILE) ANDROID_ARCH=armeabi clean
	@echo "=== arm64-v8a: clean ==="
	@$(MAKE) -f $(THIS_FILE) ANDROID_ARCH=arm64-v8a clean
	@echo "=== x86: clean ==="
	@$(MAKE) -f $(THIS_FILE) ANDROID_ARCH=x86 clean
	@echo "=== x86_64: clean ==="
	@$(MAKE) -f $(THIS_FILE) ANDROID_ARCH=x86_64 clean

android-export:
	$(GLOBAL_MKDIR) $(GLOBAL_ROOT)/build/android
	@echo '# Autogenerated by makefile' > $(GLOBAL_ROOT)/build/android/export.mk
	@echo '\nSTAPPLER_INCLUDES := $(patsubst $(GLOBAL_ROOT)/%,$(ANDROID_EXPORT_PREFIX)/%,$(filter-out $(addprefix $(GLOBAL_ROOT)/,$(OSTYPE_INCLUDE)),$(STAPPLER_INCLUDES)))' >> $(GLOBAL_ROOT)/build/android/export.mk
	@echo '\nMATERIAL_INCLUDES := $(patsubst $(GLOBAL_ROOT)/%,$(ANDROID_EXPORT_PREFIX)/%,$(filter-out $(addprefix $(GLOBAL_ROOT)/,$(OSTYPE_INCLUDE)),$(MATERIAL_INCLUDES)))' >> $(GLOBAL_ROOT)/build/android/export.mk

.PHONY: android android-clean android-export android-arm android-armv7 android-arm64 android-x86 android-x86_64