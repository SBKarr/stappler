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

BUILD_OUTDIR := $(LOCAL_OUTDIR)

GLOBAL_ROOT := $(STAPPLER_ROOT)
GLOBAL_OUTPUT := $(BUILD_OUTDIR)

THIS_FILE := $(firstword $(MAKEFILE_LIST))

include $(GLOBAL_ROOT)/make/utils/compiler.mk

ifdef IOS_ARCH

BUILD_OUTDIR := $(BUILD_OUTDIR)/$(if $(RELEASE),release,debug)/$(IOS_ARCH)

GLOBAL_OUTPUT := $(BUILD_OUTDIR)
LOCAL_OUTPUT_LIBRARY := $(if $(LOCAL_LIBRARY_NAME),lib$(LOCAL_LIBRARY_NAME).a,$(LOCAL_OUTPUT_LIBRARY))
BUILD_LIBRARY := $(BUILD_OUTDIR)/$(LOCAL_OUTPUT_LIBRARY)

include $(GLOBAL_ROOT)/make/utils/external.mk

all: static

static: .prebuild_local $(BUILD_LIBRARY)

$(BUILD_LIBRARY): $(BUILD_OBJS)
	$(GLOBAL_QUIET_LINK) $(GLOBAL_LIBTOOL) -static  $(BUILD_OBJS) $(OSTYPE_STAPPLER_LIBS_LIST) -o $(BUILD_LIBRARY)

$(BUILD_OUTDIR)/%.o: /%.cpp $(TOOLKIT_H_GCH) $(TOOLKIT_GCH) 
	$(GLOBAL_QUIET_CPP) $(GLOBAL_CPP) -MMD -MP -MF $(BUILD_OUTDIR)/$*.d $(BUILD_CXXFLAGS) -c -o $@ `$(GLOBAL_ROOT)/convert-path.sh $<`

$(BUILD_OUTDIR)/%.o: /%.mm $(TOOLKIT_H_GCH) $(TOOLKIT_GCH) 
	$(GLOBAL_QUIET_CPP) $(GLOBAL_CPP) -MMD -MP -MF $(BUILD_OUTDIR)/$*.d $(BUILD_CXXFLAGS) -c -o $@ `$(GLOBAL_ROOT)/convert-path.sh $<`

$(BUILD_OUTDIR)/%.o: /%.c $(TOOLKIT_H_GCH) $(TOOLKIT_GCH)
	$(GLOBAL_QUIET_CC) $(GLOBAL_CC) -MMD -MP -MF $(BUILD_OUTDIR)/$*.d $(BUILD_CFLAGS) -c -o $@ `$(GLOBAL_ROOT)/convert-path.sh $<`

clean_local:
	$(GLOBAL_RM) $(BUILD_LIBRARY)

.PHONY: clean_local clean .prebuild_local all static

.preclean:
	$(GLOBAL_RM) $(BUILD_LIBRARY)

.prebuild_local:
	@$(GLOBAL_MKDIR) $(BUILD_DIRS)

else

ios: .local_prebuild

ios-armv7: .local_prebuild
ios-armv7s: .local_prebuild
ios-arm64: .local_prebuild
ios-i386: .local_prebuild
ios-x86_64: .local_prebuild
ios-clean: .local_preclean

.PHONY: .local_prebuild .local_preclean

endif
