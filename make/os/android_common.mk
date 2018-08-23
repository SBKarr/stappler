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
ANDROID_EXPORT_PATH := $(if $(LOCAL_ROOT),,$(GLOBAL_ROOT)/)$(BUILD_OUTDIR)

ifdef ANDROID_EXPORT
TARGET_ARCH_ABI := $$(TARGET_ARCH_ABI)
ANDROID_ARCH := $(TARGET_ARCH_ABI)
endif

### Common
ANDROID_TOOLCHAINS := \
	$(ANDROID_TOOLCHAINS_PATH)/armeabi-v7a \
	$(ANDROID_TOOLCHAINS_PATH)/x86 \
	$(ANDROID_TOOLCHAINS_PATH)/x86_64 \
	$(ANDROID_TOOLCHAINS_PATH)/arm64-v8a \

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


android: $(ANDROID_TOOLCHAINS) android-mk
	@echo "=== armeabi-v7a: Build static library ==="
	@$(MAKE) ANDROID_ARCH=armeabi-v7a all
	@echo "=== arm64-v8a: Build static library ==="
	@$(MAKE) ANDROID_ARCH=arm64-v8a all
	@echo "=== x86: Build static library ==="
	@$(MAKE) ANDROID_ARCH=x86 all
	@echo "=== x86_64: Build static library ==="
	@$(MAKE) ANDROID_ARCH=x86_64 all


android-armv7: $(ANDROID_TOOLCHAINS_PATH)/armeabi-v7a android-mk
	@echo "=== armeabi-v7a: Build static library ==="
	@$(MAKE) ANDROID_ARCH=armeabi-v7a all


android-arm64: $(ANDROID_TOOLCHAINS_PATH)/arm64-v8a android-mk
	@echo "=== arm64-v8a: Build static library ==="
	@$(MAKE) ANDROID_ARCH=arm64-v8a all


android-x86: $(ANDROID_TOOLCHAINS_PATH)/x86 android-mk
	@echo "=== x86: Build static library ==="
	@$(MAKE) ANDROID_ARCH=x86 all


android-x86_64: $(ANDROID_TOOLCHAINS_PATH)/x86_64 android-mk
	@echo "=== x86_64: Build static library ==="
	@$(MAKE) ANDROID_ARCH=x86_64 all

android-mk:
	@$(MAKE) ANDROID_EXPORT=1 android-export

android-clean: $(ANDROID_TOOLCHAINS)
	@echo "=== armeabi-v7a: clean ==="
	@$(MAKE) ANDROID_ARCH=armeabi-v7a clean
	@echo "=== armeabi: clean ==="
	@$(MAKE) ANDROID_ARCH=armeabi clean
	@echo "=== arm64-v8a: clean ==="
	@$(MAKE) ANDROID_ARCH=arm64-v8a clean
	@echo "=== x86: clean ==="
	@$(MAKE) ANDROID_ARCH=x86 clean
	@echo "=== x86_64: clean ==="
	@$(MAKE) ANDROID_ARCH=x86_64 clean

android_make_os_includes = \
	$(abspath $(addprefix $(ANDROID_EXPORT_PREFIX)/,$(OSTYPE_INCLUDE)))\

android_make_os_prebuild = \
	$(abspath $(addprefix $(ANDROID_EXPORT_PREFIX)/,$(OSTYPE_PREBUILT_PATH)))\

android_make_stappler_includes = \
	$(patsubst $(GLOBAL_ROOT)/%,$(ANDROID_EXPORT_PREFIX)/%,\
		$(STAPPLER_INCLUDES)\
	)

android_make_material_includes = \
	$(patsubst $(GLOBAL_ROOT)/%,$(ANDROID_EXPORT_PREFIX)/%,\
		$(MATERIAL_INCLUDES)\
	)

android_make_build_includes = \
	$(patsubst $(GLOBAL_ROOT)/%,$(ANDROID_EXPORT_PREFIX)/%,\
		$(realpath $(BUILD_INCLUDES))\
	)

android_lib_list = \
	$(foreach lib,$(1),$(2)_$(basename $(notdir $(lib)))_generic)

android_lib_defs = \
	$(foreach lib,$(1),\ninclude $$(CLEAR_VARS)\nLOCAL_MODULE := $(2)_$(basename $(notdir $(lib)))_generic\nLOCAL_SRC_FILES := $(abspath $(lib))\ninclude $$(PREBUILT_STATIC_LIBRARY))

$(ANDROID_EXPORT_PATH)/Android.mk.tmp:
	@$(GLOBAL_MKDIR) $(ANDROID_EXPORT_PATH)
	@echo '# Autogenerated by makefile' > $@
	@echo 'include $$(CLEAR_VARS)' >> $@
	@echo 'LOCAL_MODULE := stappler_application_generic' >> $@
	@echo 'LOCAL_CFLAGS := -DANDROID -DUSE_FILE32API -DSTAPPLER' >> $@
	@echo 'LOCAL_EXPORT_LDLIBS := -lGLESv2 -lEGL -llog -lz -landroid' >> $@
	@echo 'LOCAL_SRC_FILES := $(realpath $(BUILD_SRCS) $(TOOLKIT_SRCS))' >> $@
	@echo 'LOCAL_C_INCLUDES := $(call android_make_material_includes) $(call android_make_build_includes) $(call android_make_os_includes)' >> $@
	@echo 'LOCAL_WHOLE_STATIC_LIBRARIES := cpufeatures $(call android_lib_list,$(OSTYPE_STAPPLER_LIBS_LIST),stappler) $(call android_lib_list,$(BUILD_LIBS_LIST),build)' >> $@
	@echo 'include $$(BUILD_STATIC_LIBRARY)' >> $@
	@echo '$(call android_lib_defs,$(OSTYPE_STAPPLER_LIBS_LIST),stappler)' >> $@
	@echo '$(call android_lib_defs,$(BUILD_LIBS_LIST),build)' >> $@
	@echo '$$(call import-module,android/cpufeatures)' >> $@

android-export: $(ANDROID_EXPORT_PATH)/Android.mk.tmp
	@if cmp -s "$(ANDROID_EXPORT_PATH)/Android.mk.tmp" "$(ANDROID_EXPORT_PATH)/Android.mk" ; then\
		echo "$(ANDROID_EXPORT_PATH)/Android.mk is up to date"; \
		$(GLOBAL_RM) $(ANDROID_EXPORT_PATH)/Android.mk.tmp; \
	else \
		$(GLOBAL_RM) $(ANDROID_EXPORT_PATH)/Android.mk; \
		mv $(ANDROID_EXPORT_PATH)/Android.mk.tmp $(ANDROID_EXPORT_PATH)/Android.mk; \
		touch $(ANDROID_EXPORT_PATH)/Android.mk; \
		echo "$(ANDROID_EXPORT_PATH)/Android.mk was updated"; \
	fi

.PHONY: android android-clean android-export android-arm android-armv7 android-arm64 android-x86 android-x86_64
