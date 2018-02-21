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

STAPPLER_OUTPUT_DIR = $(TOOLKIT_OUTPUT)/stappler
STAPPLER_OUTPUT = $(TOOLKIT_OUTPUT)/libstappler.so
STAPPLER_OUTPUT_STATIC = $(TOOLKIT_OUTPUT)/libstappler.a

STAPPLER_PRECOMPILED_HEADERS += \
	stappler/src/core/SPDefine.h \
	layout/SPLayout.h \
	common/core/SPCore.h \
	common/core/SPCommon.h

STAPPLER_SRCS_DIRS += \
	common \
	layout \
	stappler/src/actions \
	stappler/src/components \
	stappler/src/core \
	stappler/src/features \
	stappler/src/nodes

STAPPLER_SRCS_OBJS += \
	stappler/src/platform/SPPlatform.scu.cpp \
	stappler/src/SPExtra.scu.cpp \
	$(COCOS2D_STAPPLER_SRCS_OBJS)

STAPPLER_INCLUDES_DIRS += \
	common \
	layout \
	stappler/src \

STAPPLER_INCLUDES_OBJS += \
	$(COCOS2D_STAPPLER_INCLUDES_OBJS) \
	$(OSTYPE_INCLUDE)

ifdef ANDROID_ARCH

STAPPLER_SRCS_DIRS += stappler/src/platform/android

else ifdef IOS_ARCH

STAPPLER_SRCS_DIRS += $(COCOS2D_STAPPLER_SRCS_DIRS)
STAPPLER_SRCS_OBJS += stappler/src/platform/SPPlatformApple.scu.mm

else

ifeq ($(UNAME),Darwin)
STAPPLER_SRCS_DIRS += $(COCOS2D_ROOT)/cocos/platform/mac $(COCOS2D_ROOT)/cocos/platform/apple
STAPPLER_SRCS_OBJS += stappler/src/platform/SPPlatformApple.scu.mm
ifndef LOCAL_MAIN
STAPPLER_SRCS_DIRS += stappler/src/platform/mac_main
endif
else ifeq ($(UNAME),Cygwin)
ifndef LOCAL_MAIN
STAPPLER_SRCS_DIRS += stappler/src/platform/win32_main
endif
else
ifndef LOCAL_MAIN
STAPPLER_SRCS_DIRS += stappler/src/platform/linux_main
endif
endif

endif

STAPPLER_FLAGS := -DCC_STATIC

TOOLKIT_NAME := STAPPLER
TOOLKIT_TITLE := stappler

include $(GLOBAL_ROOT)/make/utils/toolkit.mk

$(STAPPLER_OUTPUT_DIR)/include/%.h : $(GLOBAL_ROOT)/%.h
	@$(GLOBAL_MKDIR) $(dir $(STAPPLER_OUTPUT_DIR)/include/$*.h)
	@cp -f $< $(STAPPLER_OUTPUT_DIR)/include/$*.h

$(STAPPLER_OUTPUT_DIR)/include/%.h.gch: $(STAPPLER_OUTPUT_DIR)/include/%.h
	$(GLOBAL_QUIET_CPP) $(GLOBAL_CPP) $(OSTYPE_GCHFLAGS) -MMD -MP -MF $(STAPPLER_OUTPUT_DIR)/include/$*.d $(STAPPLER_CXXFLAGS) -c -o $@ $<

$(STAPPLER_OUTPUT_DIR)/%.o: $(GLOBAL_ROOT)/%.cpp $(STAPPLER_H_GCH) $(STAPPLER_GCH)
	$(GLOBAL_QUIET_CPP) $(GLOBAL_CPP) -MMD -MP -MF $(STAPPLER_OUTPUT_DIR)/$*.d $(STAPPLER_CXXFLAGS) -c -o $@ $<

$(STAPPLER_OUTPUT_DIR)/%.o: $(GLOBAL_ROOT)/%.mm $(STAPPLER_H_GCH) $(STAPPLER_GCH)
	$(GLOBAL_QUIET_CPP) $(GLOBAL_CPP) -MMD -MP -MF $(STAPPLER_OUTPUT_DIR)/$*.d $(STAPPLER_CXXFLAGS) -fobjc-arc -c -o $@ $<

$(STAPPLER_OUTPUT_DIR)/%.o: $(GLOBAL_ROOT)/%.c $(STAPPLER_H_GCH) $(STAPPLER_GCH)
	$(GLOBAL_QUIET_CC) $(GLOBAL_CC) -MMD -MP -MF $(STAPPLER_OUTPUT_DIR)/$*.d $(STAPPLER_CFLAGS) -c -o $@ $<

$(STAPPLER_OUTPUT): $(STAPPLER_H_GCH) $(STAPPLER_GCH) $(STAPPLER_OBJS)
	$(GLOBAL_QUIET_LINK) $(GLOBAL_CPP) $(STAPPLER_OBJS) $(STAPPLER_LIBS) -shared $(OSTYPE_LDFLAGS) -o $(STAPPLER_OUTPUT)

$(STAPPLER_OUTPUT_STATIC) : $(STAPPLER_H_GCH) $(STAPPLER_GCH) $(STAPPLER_OBJS)
	$(GLOBAL_QUIET_LINK) $(GLOBAL_AR) $(STAPPLER_OUTPUT_STATIC) $(STAPPLER_OBJS)

libstappler: .prebuild_stappler $(STAPPLER_OUTPUT_STATIC)

.prebuild_stappler:
	@echo "=== Build libstappler ==="
	@$(GLOBAL_MKDIR) $(STAPPLER_DIRS)

.PHONY: .prebuild_stappler libstappler
