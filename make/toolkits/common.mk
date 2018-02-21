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

COMMON_OUTPUT_DIR = $(TOOLKIT_OUTPUT)/common
COMMON_OUTPUT = $(TOOLKIT_OUTPUT)/libcommon.so
COMMON_OUTPUT_STATIC = $(TOOLKIT_OUTPUT)/libcommon.a

COMMON_FLAGS := -DNOCC

COMMON_PRECOMPILED_HEADERS += \
	common/core/SPCore.h \
	common/core/SPCommon.h

COMMON_SRCS_DIRS += common
COMMON_SRCS_OBJS += 
COMMON_INCLUDES_DIRS += common
COMMON_INCLUDES_OBJS += $(OSTYPE_INCLUDE)

TOOLKIT_NAME := COMMON
TOOLKIT_TITLE := common

include $(GLOBAL_ROOT)/make/utils/toolkit.mk

$(COMMON_OUTPUT_DIR)/include/%.h : $(GLOBAL_ROOT)/%.h
	@$(GLOBAL_MKDIR) $(dir $(COMMON_OUTPUT_DIR)/include/$*.h)
	@cp -f $< $(COMMON_OUTPUT_DIR)/include/$*.h

$(COMMON_OUTPUT_DIR)/include/%.h.gch: $(COMMON_OUTPUT_DIR)/include/%.h
	$(GLOBAL_QUIET_CPP) $(GLOBAL_CPP) $(OSTYPE_GCHFLAGS) -MMD -MP -MF $(COMMON_OUTPUT_DIR)/include/$*.d $(COMMON_CXXFLAGS) -c -o $@ $<

$(COMMON_OUTPUT_DIR)/%.o: $(GLOBAL_ROOT)/%.cpp $(COMMON_H_GCH) $(COMMON_GCH)
	$(GLOBAL_QUIET_CPP) $(GLOBAL_CPP) -MMD -MP -MF $(COMMON_OUTPUT_DIR)/$*.d $(COMMON_CXXFLAGS) -c -o $@ $<

$(COMMON_OUTPUT_DIR)/%.o: $(GLOBAL_ROOT)/%.mm $(COMMON_H_GCH) $(COMMON_GCH)
	$(GLOBAL_QUIET_CPP) $(GLOBAL_CPP) -MMD -MP -MF $(COMMON_OUTPUT_DIR)/$*.d $(COMMON_CXXFLAGS) -fobjc-arc -c -o $@ $<

$(COMMON_OUTPUT_DIR)/%.o: $(GLOBAL_ROOT)/%.c $($(TOOLKIT_NAME)_H_GCH) $($(TOOLKIT_NAME)_GCH)
	$(GLOBAL_QUIET_CC) $(GLOBAL_CC) -MMD -MP -MF $(COMMON_OUTPUT_DIR)/$*.d $(COMMON_CFLAGS) -c -o $@ $<

$(COMMON_OUTPUT): $(COMMON_H_GCH) $(COMMON_GCH) $(COMMON_OBJS)
	$(GLOBAL_QUIET_LINK) $(GLOBAL_CPP)  $(COMMON_OBJS) $(COMMON_LIBS) -shared $(OSTYPE_LDFLAGS) -o $(COMMON_OUTPUT)

$(COMMON_OUTPUT_STATIC) : $(COMMON_H_GCH) $(COMMON_GCH) $(COMMON_OBJS)
	$(GLOBAL_QUIET_LINK) $(GLOBAL_AR) $(COMMON_OUTPUT_STATIC) $(COMMON_OBJS)

libcommon: .prebuild_common $(COMMON_OUTPUT_STATIC)

.prebuild_common:
	@echo "=== Build libcommon ==="
	@$(GLOBAL_MKDIR) $(COMMON_DIRS)

.PHONY: .prebuild_common libcommon
