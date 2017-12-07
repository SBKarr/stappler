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
	layout/style \
	layout/types \
	layout/vg \
	stappler/src/core \
	stappler/src/features/ime \
	stappler/src/features/networking \
	stappler/src/features/storage \
	stappler/src/features/threads \
	stappler/src/platform/universal \
	stappler/src/platform/linux \
	stappler/src/platform/mac \
	$(COCOS2D_CLI_SRCS_DIRS)

CLI_SRCS_OBJS += \
	layout/document/SLNode.cpp \
	layout/document/SLParser.cpp \
	layout/document/SLReader.cpp \
	$(COCOS2D_CLI_SRCS_OBJS)

CLI_INCLUDES_DIRS += \
	common \
	layout \
	stappler/src

CLI_INCLUDES_OBJS += \
	$(COCOS2D_CLI_INCLUDES_OBJS) \
	$(OSTYPE_INCLUDE)

CLI_LIBS += -L$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH) $(OSTYPE_CLI_LIBS)

CLI_SRCS := \
	$(foreach dir,$(CLI_SRCS_DIRS),$(shell find $(GLOBAL_ROOT)/$(dir) -name '*.c*')) \
	$(addprefix $(GLOBAL_ROOT)/,$(CLI_SRCS_OBJS))

ifeq ($(OBJC),1)
CLI_SRCS += $(foreach dir,$(CLI_SRCS_DIRS),$(shell find $(GLOBAL_ROOT)/$(dir) -name '*.mm'))
CLI_SRCS += $(addprefix $(GLOBAL_ROOT)/,$(COCOS2D_ROOT)/cocos/platform/apple/CCFileUtils-apple.mm)
endif

CLI_INCLUDES := \
	$(foreach dir,$(CLI_INCLUDES_DIRS),$(shell find $(GLOBAL_ROOT)/$(dir) -type d)) \
	$(addprefix $(GLOBAL_ROOT)/,$(CLI_INCLUDES_OBJS))

CLI_GCH := $(addprefix $(GLOBAL_ROOT)/,$(CLI_PRECOMPILED_HEADERS))
CLI_H_GCH := $(patsubst $(GLOBAL_ROOT)/%,$(CLI_OUTPUT_DIR)/include/%,$(CLI_GCH))
CLI_GCH := $(addsuffix .gch,$(CLI_H_GCH))

CLI_OBJS := $(patsubst %.mm,%.o,$(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(patsubst $(GLOBAL_ROOT)/%,$(CLI_OUTPUT_DIR)/%,$(CLI_SRCS)))))
CLI_DIRS := $(sort $(dir $(CLI_OBJS))) $(sort $(dir $(CLI_GCH)))

CLI_INPUT_CFLAGS := $(addprefix -I,$(sort $(dir $(CLI_GCH)))) $(addprefix -I,$(CLI_INCLUDES))

CLI_CXXFLAGS := $(GLOBAL_CXXFLAGS) $(CLI_FLAGS) $(CLI_INPUT_CFLAGS)
CLI_CFLAGS := $(GLOBAL_CFLAGS) $(CLI_FLAGS) $(CLI_INPUT_CFLAGS)


# Progress counter
CLI_COUNTER := 0
CLI_WORDS := $(words $(CLI_GCH) $(CLI_OBJS))

define CLI_template =
$(eval CLI_COUNTER=$(shell echo $$(($(CLI_COUNTER)+1))))
$(1):BUILD_CURRENT_COUNTER:=$(CLI_COUNTER)
$(1):BUILD_FILES_COUNTER := $(CLI_WORDS)
$(1):BUILD_LIBRARY := cli
endef

$(foreach obj,$(CLI_GCH) $(CLI_OBJS),$(eval $(call CLI_template,$(obj))))


-include $(patsubst %.o,%.d,$(CLI_OBJS))
-include $(patsubst %.gch,%.d,$(CLI_GCH))

$(CLI_OUTPUT_DIR)/include/%.h : $(GLOBAL_ROOT)/%.h
	@$(GLOBAL_MKDIR) $(dir $(CLI_OUTPUT_DIR)/include/$*.h)
	@cp -f $< $(CLI_OUTPUT_DIR)/include/$*.h

$(CLI_OUTPUT_DIR)/include/%.h.gch: $(CLI_OUTPUT_DIR)/include/%.h
	$(GLOBAL_QUIET_CPP) $(GLOBAL_CPP) $(OSTYPE_GCHFLAGS) -MMD -MP -MF $(CLI_OUTPUT_DIR)/include/$*.d $(CLI_CXXFLAGS) -c -o $@ $<

$(CLI_OUTPUT_DIR)/%.o: $(GLOBAL_ROOT)/%.cpp $(CLI_GCH)
	$(GLOBAL_QUIET_CPP) $(GLOBAL_CPP) -MMD -MP -MF $(CLI_OUTPUT_DIR)/$*.d $(CLI_CXXFLAGS) -c -o $@ $<

$(CLI_OUTPUT_DIR)/%.o: $(GLOBAL_ROOT)/%.mm $(CLI_GCH)
	$(GLOBAL_QUIET_CPP) $(GLOBAL_CPP) -MMD -MP -MF $(CLI_OUTPUT_DIR)/$*.d $(CLI_CXXFLAGS) -fobjc-arc -c -o $@ $<

$(CLI_OUTPUT_DIR)/%.o: $(GLOBAL_ROOT)/%.c $(CLI_GCH)
	$(GLOBAL_QUIET_CC) $(GLOBAL_CC) -MMD -MP -MF $(CLI_OUTPUT_DIR)/$*.d $(CLI_CFLAGS) -c -o $@ $<

$(CLI_OUTPUT): $(CLI_H_GCH) $(CLI_GCH) $(CLI_OBJS)
	$(GLOBAL_QUIET_LINK) $(GLOBAL_CPP)  $(CLI_OBJS) $(CLI_LIBS) -shared $(OSTYPE_LDFLAGS) -o $(CLI_OUTPUT)

$(CLI_OUTPUT_STATIC) : $(CLI_H_GCH) $(CLI_GCH) $(CLI_OBJS)
	$(GLOBAL_QUIET_LINK) $(GLOBAL_AR) $(CLI_OUTPUT_STATIC) $(CLI_OBJS)

libcli: .prebuild_cli $(CLI_OUTPUT) $(CLI_OUTPUT_STATIC)

.prebuild_cli:
	@echo "=== Build libcli ==="
	@$(GLOBAL_MKDIR) $(CLI_DIRS)

.PHONY: .prebuild_cli libcli
