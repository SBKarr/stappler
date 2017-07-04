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

.DEFAULT_GOAL := local

IS_LOCAL_BUILD := 1

BUILD_OUTDIR := $(LOCAL_OUTDIR)/objs

GLOBAL_ROOT := $(STAPPLER_ROOT)
GLOBAL_OUTPUT := $(BUILD_OUTDIR)

ifndef COCOS2D_ROOT
COCOS2D_ROOT := libs/external/stappler-cocos2d-x
endif

include $(GLOBAL_ROOT)/make/compiler.mk

ifdef RELEASE
BUILD_OUTDIR := $(BUILD_OUTDIR)/$(GLOBAL_CC)/release
else
BUILD_OUTDIR := $(BUILD_OUTDIR)/$(GLOBAL_CC)/debug
endif

BUILD_OUTDIR := $(BUILD_OUTDIR)/local


BUILD_SRCS := \
	$(foreach dir,$(LOCAL_SRCS_DIRS),$(shell find $(LOCAL_ROOT)/$(dir) -name '*.c*')) \
	$(addprefix $(LOCAL_ROOT)/,$(LOCAL_SRCS_OBJS))

ifeq ($(OBJC),1)
BUILD_SRCS += $(foreach dir,$(LOCAL_SRCS_DIRS),$(shell find $(LOCAL_ROOT)/$(dir) -name '*.mm'))
endif

BUILD_INCLUDES := \
	$(foreach dir,$(LOCAL_INCLUDES_DIRS),$(shell find $(LOCAL_ROOT)/$(dir) -type d)) \
	$(addprefix $(LOCAL_ROOT)/,$(LOCAL_INCLUDES_OBJS)) \
	$(LOCAL_ABSOLUTE_INCLUDES)


# build local object list
BUILD_SRCS := $(realpath $(BUILD_SRCS))
BUILD_SRCS := $(BUILD_SRCS:.c=.o)
BUILD_SRCS := $(BUILD_SRCS:.cpp=.o)
BUILD_SRCS := $(BUILD_SRCS:.mm=.o)

BUILD_OBJS := $(addprefix $(BUILD_OUTDIR),$(BUILD_SRCS))

ifdef LOCAL_MAIN
BUILD_MAIN_SRC := $(realpath $(addprefix $(LOCAL_ROOT)/,$(LOCAL_MAIN)))
BUILD_MAIN_SRC := $(BUILD_MAIN_SRC:.c=.o)
BUILD_MAIN_SRC := $(BUILD_MAIN_SRC:.cpp=.o)
BUILD_MAIN_SRC := $(BUILD_MAIN_SRC:.mm=.o)
BUILD_MAIN_OBJ := $(addprefix $(BUILD_OUTDIR),$(BUILD_MAIN_SRC))
else
BUILD_MAIN_OBJ := 
endif

ifdef LOCAL_TOOLKIT_OUTPUT
#TOOLKIT_OUTPUT := $(GLOBAL_ROOT)/build
#else
TOOLKIT_OUTPUT := $(LOCAL_TOOLKIT_OUTPUT)
endif

include $(GLOBAL_ROOT)/make/build.mk

ifndef LOCAL_TOOLKIT
LOCAL_TOOLKIT := material
endif

ifeq ($(LOCAL_TOOLKIT),material)
TOOLKIT_OBJS := $(STAPPLER_OBJS) $(MATERIAL_OBJS)
TOOLKIT_CFLAGS := $(MATERIAL_CFLAGS)
TOOLKIT_CXXFLAGS := $(MATERIAL_CXXFLAGS)
TOOLKIT_LIBS := $(STAPPLER_LIBS)
TOOLKIT_LIB_FLAGS := $(OSTYPE_LDFLAGS)
TOOLKIT_H_GCH := $(MATERIAL_H_GCH)
TOOLKIT_GCH := $(MATERIAL_GCH)
endif
ifeq ($(LOCAL_TOOLKIT),stappler)
TOOLKIT_OBJS := $(STAPPLER_OBJS)
TOOLKIT_CFLAGS := $(STAPPLER_CFLAGS)
TOOLKIT_CXXFLAGS := $(STAPPLER_CXXFLAGS)
TOOLKIT_LIBS := $(STAPPLER_LIBS)
TOOLKIT_LIB_FLAGS := $(OSTYPE_LDFLAGS)
TOOLKIT_H_GCH := $(STAPPLER_H_GCH)
TOOLKIT_GCH := $(STAPPLER_GCH)
endif
ifeq ($(LOCAL_TOOLKIT),cli)
TOOLKIT_OBJS := $(CLI_OBJS)
TOOLKIT_CFLAGS := $(CLI_CFLAGS)
TOOLKIT_CXXFLAGS := $(CLI_CXXFLAGS)
TOOLKIT_LIBS := $(CLI_LIBS)
TOOLKIT_LIB_FLAGS := $(OSTYPE_LDFLAGS)
TOOLKIT_H_GCH := $(CLI_H_GCH)
TOOLKIT_GCH := $(CLI_GCH)
endif
ifeq ($(LOCAL_TOOLKIT),common)
TOOLKIT_OBJS := $(SPMIN_OBJS)
TOOLKIT_CFLAGS := $(SPMIN_CFLAGS)
TOOLKIT_CXXFLAGS := $(SPMIN_CXXFLAGS)
TOOLKIT_LIBS := $(SPMIN_LIBS)
TOOLKIT_LIB_FLAGS := $(OSTYPE_LDFLAGS)
TOOLKIT_H_GCH := $(SPMIN_H_GCH)
TOOLKIT_GCH := $(SPMIN_GCH)
endif
ifeq ($(LOCAL_TOOLKIT),serenity)
TOOLKIT_CFLAGS := $(SERENITY_CFLAGS)
TOOLKIT_CXXFLAGS := $(SERENITY_CXXFLAGS)
TOOLKIT_LIBS := $(SERENITY_LIBS)
TOOLKIT_LIB_FLAGS := 
TOOLKIT_H_GCH := $(SERENITY_H_GCH)
TOOLKIT_GCH := $(SERENITY_GCH)
endif


BUILD_CFLAGS += $(LOCAL_CFLAGS) $(TOOLKIT_CFLAGS)
BUILD_CXXFLAGS += $(LOCAL_CFLAGS) $(LOCAL_CXXFLAGS) $(TOOLKIT_CXXFLAGS)

BUILD_OBJS += $(TOOLKIT_OBJS)
BUILD_DIRS := $(sort $(dir $(BUILD_OBJS) $(BUILD_MAIN_OBJ)))

BUILD_CFLAGS += $(addprefix -I,$(BUILD_INCLUDES))
BUILD_CXXFLAGS += $(addprefix -I,$(BUILD_INCLUDES))

BUILD_LIBS := $(LOCAL_LIBS) $(TOOLKIT_LIBS)

-include $(patsubst %.o,%.d,$(BUILD_OBJS) $(BUILD_MAIN_OBJ))

all: local

local: .prebuild_local .local_prebuild $(LOCAL_OUTPUT_LIBRARY) $(LOCAL_OUTPUT_EXECUTABLE)
.preclean: .local_preclean

$(LOCAL_OUTPUT_EXECUTABLE) : $(BUILD_OBJS) $(BUILD_MAIN_OBJ)
	$(GLOBAL_QUIET_LINK) $(GLOBAL_CPP) $(BUILD_OBJS) $(BUILD_MAIN_OBJ) $(BUILD_LIBS) $(TOOLKIT_LIB_FLAGS) $(OSTYPE_EXEC_FLAGS) -o $(LOCAL_OUTPUT_EXECUTABLE)

$(LOCAL_OUTPUT_LIBRARY): $(BUILD_OBJS)
	$(GLOBAL_QUIET_LINK) $(GLOBAL_CPP) -shared  $(BUILD_OBJS) $(BUILD_LIBS) $(TOOLKIT_LIB_FLAGS) $(OSTYPE_EXEC_FLAGS) -o $(LOCAL_OUTPUT_LIBRARY)

$(BUILD_OUTDIR)/%.o: /%.cpp $(TOOLKIT_H_GCH) $(TOOLKIT_GCH) 
	$(GLOBAL_QUIET_CPP) $(GLOBAL_CPP) -MMD -MP -MF $(BUILD_OUTDIR)/$*.d $(BUILD_CXXFLAGS) -c -o $@ `$(GLOBAL_ROOT)/convert-path.sh $<`

$(BUILD_OUTDIR)/%.o: /%.mm $(TOOLKIT_H_GCH) $(TOOLKIT_GCH) 
	$(GLOBAL_QUIET_CPP) $(GLOBAL_CPP) -MMD -MP -MF $(BUILD_OUTDIR)/$*.d $(BUILD_CXXFLAGS) -c -o $@ `$(GLOBAL_ROOT)/convert-path.sh $<`

$(BUILD_OUTDIR)/%.o: $(LOCAL_ROOT)/%.c $(TOOLKIT_H_GCH) $(TOOLKIT_GCH)
	$(GLOBAL_QUIET_CC) $(GLOBAL_CC) -MMD -MP -MF $(BUILD_OUTDIR)/$*.d $(BUILD_CFLAGS) -c -o $@ `$(GLOBAL_ROOT)/convert-path.sh $<`

.PHONY: clean .prebuild_local .local_prebuild local .local_preclean

.preclean:
	$(GLOBAL_RM) $(LOCAL_OUTPUT_EXECUTABLE) $(LOCAL_OUTPUT_LIBRARY)
	
.prebuild_local:
	@echo "=== Begin build ==="
	@$(GLOBAL_MKDIR) $(BUILD_DIRS)
