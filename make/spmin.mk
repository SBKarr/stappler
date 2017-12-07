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

SPMIN_OUTPUT_DIR = $(TOOLKIT_OUTPUT)/common
SPMIN_OUTPUT = $(TOOLKIT_OUTPUT)/libcommon.so
SPMIN_OUTPUT_STATIC = $(TOOLKIT_OUTPUT)/libcommon.a

SPMIN_FLAGS := -DNOCC

SPMIN_PRECOMPILED_HEADERS += \
	common/core/SPCore.h \
	common/core/SPCommon.h

SPMIN_SRCS_DIRS += common
SPMIN_SRCS_OBJS += 
SPMIN_INCLUDES_DIRS += common
SPMIN_INCLUDES_OBJS += $(OSTYPE_INCLUDE)

SPMIN_LIBS += -L$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH) $(OSTYPE_COMMON_LIBS)

SPMIN_SRCS := \
	$(foreach dir,$(SPMIN_SRCS_DIRS),$(shell find $(GLOBAL_ROOT)/$(dir) -name '*.c*')) \
	$(addprefix $(GLOBAL_ROOT)/,$(SPMIN_SRCS_OBJS))

ifeq ($(OBJC),1)
SPMIN_SRCS += $(foreach dir,$(SPMIN_SRCS_DIRS),$(shell find $(GLOBAL_ROOT)/$(dir) -name '*.mm'))
endif

SPMIN_INCLUDES := \
	$(foreach dir,$(SPMIN_INCLUDES_DIRS),$(shell find $(GLOBAL_ROOT)/$(dir) -type d)) \
	$(addprefix $(GLOBAL_ROOT)/,$(SPMIN_INCLUDES_OBJS))

SPMIN_GCH := $(addsuffix .gch,$(addprefix $(GLOBAL_ROOT)/,$(SPMIN_PRECOMPILED_HEADERS)))
SPMIN_GCH := $(patsubst $(GLOBAL_ROOT)/%,$(SPMIN_OUTPUT_DIR)/include/%,$(SPMIN_GCH))

SPMIN_GCH := $(addprefix $(GLOBAL_ROOT)/,$(SPMIN_PRECOMPILED_HEADERS))
SPMIN_H_GCH := $(patsubst $(GLOBAL_ROOT)/%,$(SPMIN_OUTPUT_DIR)/include/%,$(SPMIN_GCH))
SPMIN_GCH := $(addsuffix .gch,$(SPMIN_H_GCH))

SPMIN_OBJS := $(patsubst %.mm,%.o,$(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(patsubst $(GLOBAL_ROOT)/%,$(SPMIN_OUTPUT_DIR)/%,$(SPMIN_SRCS)))))
SPMIN_DIRS := $(sort $(dir $(SPMIN_OBJS))) $(sort $(dir $(SPMIN_GCH)))

SPMIN_INPUT_CFLAGS := $(addprefix -I,$(sort $(dir $(SPMIN_GCH)))) $(addprefix -I,$(SPMIN_INCLUDES))

SPMIN_CXXFLAGS := $(GLOBAL_CXXFLAGS) $(SPMIN_FLAGS) $(SPMIN_INPUT_CFLAGS)
SPMIN_CFLAGS := $(GLOBAL_CFLAGS) $(SPMIN_FLAGS) $(SPMIN_INPUT_CFLAGS)


# Progress counter
SPMIN_COUNTER := 0
SPMIN_WORDS := $(words $(SPMIN_GCH) $(SPMIN_OBJS))

define SPMIN_template =
$(eval SPMIN_COUNTER=$(shell echo $$(($(SPMIN_COUNTER)+1))))
$(1):BUILD_CURRENT_COUNTER:=$(SPMIN_COUNTER)
$(1):BUILD_FILES_COUNTER := $(SPMIN_WORDS)
$(1):BUILD_LIBRARY := common
endef

$(foreach obj,$(SPMIN_GCH) $(SPMIN_OBJS),$(eval $(call SPMIN_template,$(obj))))


# include dependencies
-include $(patsubst %.o,%.d,$(SPMIN_OBJS))
-include $(patsubst %.gch,%.d,$(SPMIN_GCH))

$(SPMIN_OUTPUT_DIR)/include/%.h : $(GLOBAL_ROOT)/%.h
	@$(GLOBAL_MKDIR) $(dir $(SPMIN_OUTPUT_DIR)/include/$*.h)
	@cp -f $< $(SPMIN_OUTPUT_DIR)/include/$*.h

$(SPMIN_OUTPUT_DIR)/include/%.h.gch: $(SPMIN_OUTPUT_DIR)/include/%.h
	$(GLOBAL_QUIET_CPP) $(GLOBAL_CPP) $(OSTYPE_GCHFLAGS) -MMD -MP -MF $(SPMIN_OUTPUT_DIR)/include/$*.d $(SPMIN_CXXFLAGS) -c -o $@ $<

$(SPMIN_OUTPUT_DIR)/%.o: $(GLOBAL_ROOT)/%.cpp $(SPMIN_GCH)
	$(GLOBAL_QUIET_CPP) $(GLOBAL_CPP) -MMD -MP -MF $(SPMIN_OUTPUT_DIR)/$*.d $(SPMIN_CXXFLAGS) -c -o $@ $<

$(SPMIN_OUTPUT_DIR)/%.o: $(GLOBAL_ROOT)/%.mm $(SPMIN_GCH)
	$(GLOBAL_QUIET_CPP) $(GLOBAL_CPP) -MMD -MP -MF $(SPMIN_OUTPUT_DIR)/$*.d $(SPMIN_CXXFLAGS) -fobjc-arc -c -o $@ $<

$(SPMIN_OUTPUT_DIR)/%.o: $(GLOBAL_ROOT)/%.c $(SPMIN_GCH)
	$(GLOBAL_QUIET_CC) $(GLOBAL_CC) -MMD -MP -MF $(SPMIN_OUTPUT_DIR)/$*.d $(SPMIN_CFLAGS) -c -o $@ $<

$(SPMIN_OUTPUT): $(SPMIN_H_GCH) $(SPMIN_GCH) $(SPMIN_OBJS)
	$(GLOBAL_QUIET_LINK) $(GLOBAL_CPP)  $(SPMIN_OBJS) $(SPMIN_LIBS) -shared $(OSTYPE_LDFLAGS) -o $(SPMIN_OUTPUT)

$(SPMIN_OUTPUT_STATIC) : $(SPMIN_H_GCH) $(SPMIN_GCH) $(SPMIN_OBJS)
	$(GLOBAL_QUIET_LINK) $(GLOBAL_AR) $(SPMIN_OUTPUT_STATIC) $(SPMIN_OBJS)

libspmin: .prebuild_spmin $(SPMIN_OUTPUT) $(SPMIN_OUTPUT_STATIC)
libcommon: .prebuild_spmin $(SPMIN_OUTPUT) $(SPMIN_OUTPUT_STATIC)

.prebuild_spmin:
	@echo "=== Build libspmin ==="
	@$(GLOBAL_MKDIR) $(SPMIN_DIRS)

.PHONY: .prebuild_spmin libspmin libcommon
