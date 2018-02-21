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

CLI_OUTPUT_DIR = $(TOOLKIT_OUTPUT)/cli
CLI_OUTPUT = $(TOOLKIT_OUTPUT)/libcli.so
CLI_OUTPUT_STATIC = $(TOOLKIT_OUTPUT)/libcli.a

CLI_FLAGS := -DSP_RESTRICT -DSP_DRAW=0 -DSP_NO_STOREKIT -DCC_STATIC

CLI_PRECOMPILED_HEADERS += \
	stappler/src/core/SPDefine.h \
	layout/SPLayout.h \
	common/core/SPCore.h \
	common/core/SPCommon.h

CLI_SRCS_DIRS += \
	common \
	layout/document \
	layout/types \
	layout/vg \
	stappler/src/core \
	$(COCOS2D_CLI_SRCS_DIRS)

CLI_SRCS_OBJS += \
	stappler/src/platform/SPPlatform.scu.cpp \
	stappler/src/features/SPBasicFeatures.scu.cpp \
	$(COCOS2D_CLI_SRCS_OBJS)

CLI_INCLUDES_DIRS += \
	common \
	layout \
	stappler/src

CLI_INCLUDES_OBJS += \
	$(COCOS2D_CLI_INCLUDES_OBJS) \
	$(OSTYPE_INCLUDE)

TOOLKIT_NAME := CLI
TOOLKIT_TITLE := cli

include $(GLOBAL_ROOT)/make/utils/toolkit.mk

$(CLI_OUTPUT_DIR)/include/%.h : $(GLOBAL_ROOT)/%.h
	@$(GLOBAL_MKDIR) $(dir $(CLI_OUTPUT_DIR)/include/$*.h)
	@cp -f $< $(CLI_OUTPUT_DIR)/include/$*.h

$(CLI_OUTPUT_DIR)/include/%.h.gch: $(CLI_OUTPUT_DIR)/include/%.h
	$(GLOBAL_QUIET_CPP) $(GLOBAL_CPP) $(OSTYPE_GCHFLAGS) -MMD -MP -MF $(CLI_OUTPUT_DIR)/include/$*.d $(CLI_CXXFLAGS) -c -o $@ $<

$(CLI_OUTPUT_DIR)/%.o: $(GLOBAL_ROOT)/%.cpp $(CLI_H_GCH) $(CLI_GCH)
	$(GLOBAL_QUIET_CPP) $(GLOBAL_CPP) -MMD -MP -MF $(CLI_OUTPUT_DIR)/$*.d $(CLI_CXXFLAGS) -c -o $@ $<

$(CLI_OUTPUT_DIR)/%.o: $(GLOBAL_ROOT)/%.mm $(CLI_H_GCH) $(CLI_GCH)
	$(GLOBAL_QUIET_CPP) $(GLOBAL_CPP) -MMD -MP -MF $(CLI_OUTPUT_DIR)/$*.d $(CLI_CXXFLAGS) -fobjc-arc -c -o $@ $<

$(CLI_OUTPUT_DIR)/%.o: $(GLOBAL_ROOT)/%.c $($(TOOLKIT_NAME)_H_GCH) $($(TOOLKIT_NAME)_GCH)
	$(GLOBAL_QUIET_CC) $(GLOBAL_CC) -MMD -MP -MF $(CLI_OUTPUT_DIR)/$*.d $(CLI_CFLAGS) -c -o $@ $<

$(CLI_OUTPUT): $(CLI_H_GCH) $(CLI_GCH) $(CLI_OBJS)
	$(GLOBAL_QUIET_LINK) $(GLOBAL_CPP)  $(CLI_OBJS) $(CLI_LIBS) -shared $(OSTYPE_LDFLAGS) -o $(CLI_OUTPUT)

$(CLI_OUTPUT_STATIC) : $(CLI_H_GCH) $(CLI_GCH) $(CLI_OBJS)
	$(GLOBAL_QUIET_LINK) $(GLOBAL_AR) $(CLI_OUTPUT_STATIC) $(CLI_OBJS)

libcli: .prebuild_cli $(CLI_OUTPUT_STATIC)

.prebuild_cli:
	@echo "=== Build libcli ==="
	@$(GLOBAL_MKDIR) $(CLI_DIRS)

.PHONY: .prebuild_cli libcli
