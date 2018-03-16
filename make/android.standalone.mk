# Copyright (c) 2018 Roman Katuntsev <sbkarr@stappler.org>
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

.DEFAULT_GOAL := all

IS_LOCAL_BUILD := 1

BUILD_OUTDIR := $(LOCAL_OUTDIR)/android

GLOBAL_ROOT := $(STAPPLER_ROOT)
GLOBAL_OUTPUT := $(BUILD_OUTDIR)

THIS_FILE := $(firstword $(MAKEFILE_LIST))

include $(GLOBAL_ROOT)/make/utils/compiler.mk

ifdef ANDROID_ARCH

BUILD_OUTDIR := $(BUILD_OUTDIR)/$(if $(RELEASE),release,debug)/$(ANDROID_ARCH)
GLOBAL_OUTPUT := $(BUILD_OUTDIR)
BUILD_LIBRARY := $(BUILD_OUTDIR)/$(LOCAL_LIBRARY).so
BUILD_STATIC := $(BUILD_OUTDIR)/$(LOCAL_LIBRARY).a

ifndef ANDROID_EXPORT
LOCAL_SRCS_OBJS += $(realpath $(NDK)/sources/android/cpufeatures/cpu-features.c)
endif

include $(GLOBAL_ROOT)/make/utils/external.mk

all: $(BUILD_LIBRARY) $(BUILD_STATIC)

BUILD_LIBS += -static-libstdc++ -lc++_static -lc++abi

$(BUILD_LIBRARY): $(BUILD_OBJS)
	$(GLOBAL_QUIET_LINK) $(GLOBAL_CPP) -shared  $(BUILD_OBJS) $(BUILD_LIBS) $(TOOLKIT_LIB_FLAGS) $(OSTYPE_EXEC_FLAGS) -o $(BUILD_LIBRARY)

$(BUILD_STATIC): $(BUILD_OBJS)
	@$(GLOBAL_AR) cqT $(BUILD_STATIC) $(BUILD_OBJS) $(OSTYPE_STAPPLER_LIBS_LIST)

$(BUILD_OUTDIR)/%.o: /%.cpp $(TOOLKIT_H_GCH) $(TOOLKIT_GCH)
	$(call sp_compile_cpp,$(BUILD_CXXFLAGS))

$(BUILD_OUTDIR)/%.o: /%.mm $(TOOLKIT_H_GCH) $(TOOLKIT_GCH)
	$(call sp_compile_mm,$(BUILD_CXXFLAGS))

$(BUILD_OUTDIR)/%.o: /%.c $(TOOLKIT_H_GCH) $(TOOLKIT_GCH)
	$(call sp_compile_c,$(BUILD_CFLAGS))

clean_local:
	$(GLOBAL_RM) $(BUILD_LIBRARY)

.PHONY: clean_local clean .prebuild_local all static

.preclean:
	$(GLOBAL_RM) $(BUILD_LIBRARY)

.prebuild_local:
	@$(GLOBAL_MKDIR) $(BUILD_DIRS)

else

android: .local_prebuild

android-armv7: .local_prebuild
android-arm64: .local_prebuild
android-x86: .local_prebuild
android-x86_64: .local_prebuild
android-clean: .local_preclean

.PHONY: .local_prebuild .local_preclean

endif
