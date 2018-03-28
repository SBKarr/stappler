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

GLOBAL_ROOT ?= .
COCOS2D_ROOT ?= libs/external/stappler-cocos2d-x
TOOLKIT_OUTPUT ?= $(GLOBAL_ROOT)/build

UNAME := $(shell uname)
ifneq ($(UNAME),Darwin)
UNAME := $(shell uname -o)
endif

include $(GLOBAL_ROOT)/make/os/android_common.mk
include $(GLOBAL_ROOT)/make/os/ios_common.mk

ifdef ANDROID_ARCH

include $(GLOBAL_ROOT)/make/os/android.mk

else ifdef IOS_ARCH

include $(GLOBAL_ROOT)/make/os/ios.mk

else # Begin desktop arch

ifeq ($(UNAME),Darwin)
include $(GLOBAL_ROOT)/make/os/darwin.mk
else ifeq ($(UNAME),Cygwin)
include $(GLOBAL_ROOT)/make/os/cygwin.mk
else
include $(GLOBAL_ROOT)/make/os/linux.mk
endif

ifndef GLOBAL_CPP

ifdef CLANG
ifeq ($(CLANG),1)
GLOBAL_CPP := clang++
else
GLOBAL_CPP := clang++-$(CLANG)
endif # ifeq ($(CLANG),1)
OSTYPE_GCHFLAGS := -x c++-header
else
ifdef MINGW
GLOBAL_CPP := $(MINGW)-g++
else
GLOBAL_CPP := g++
endif # ifdef MINGW
endif # ifdef CLANG

endif # ifndef GLOBAL_CPP

ifndef GLOBAL_CC
ifdef CLANG
ifeq ($(CLANG),1)
GLOBAL_CC := clang
else
GLOBAL_CC := clang-$(CLANG)
endif # ifeq ($(CLANG),1)
OSTYPE_GCHFLAGS := -x c++-header
else
ifdef MINGW
GLOBAL_CC := $(MINGW)-gcc
else
GLOBAL_CC := gcc
endif # ifdef MINGW
endif # ifdef CLANG
endif # ifndef GLOBAL_CC

ifdef RELEASE
TOOLKIT_OUTPUT := $(TOOLKIT_OUTPUT)/$(notdir $(GLOBAL_CC))/release
GLOBAL_CFLAGS := -Os -DNDEBUG $(OSTYPE_CFLAGS) $(GLOBAL_CFLAGS)
else
TOOLKIT_OUTPUT := $(TOOLKIT_OUTPUT)/$(notdir $(GLOBAL_CC))/debug
GLOBAL_CFLAGS := -g -DDEBUG -DCOCOS2D_DEBUG=1 $(OSTYPE_CFLAGS) $(GLOBAL_CFLAGS)
endif # ifdef RELEASE

endif # End desktop arch

GLOBAL_CXXFLAGS := $(GLOBAL_CFLAGS) -DSTAPPLER -std=gnu++14 $(OSTYPE_CPPFLAGS)
GLOBAL_CFLAGS := $(GLOBAL_CFLAGS) -DSTAPPLER -std=gnu11

GLOBAL_RM ?= rm -f
GLOBAL_CP ?= cp -f
GLOBAL_MAKE ?= make
GLOBAL_MKDIR ?= mkdir -p
GLOBAL_AR ?= ar rcs

ifeq ($(shell uname),Darwin)
sp_convert_path = $(1)
else
ifeq ($(shell uname -o),Cygwin)
sp_convert_path = $(shell cygpath -w $1)
else
sp_convert_path = $(1)
endif
endif

sp_compile_gch = $(GLOBAL_QUIET_CPP) $(GLOBAL_MKDIR) $(dir $@); $(GLOBAL_CPP) $(OSTYPE_GCHFLAGS) -MMD -MP -MF $(dir $@)/$(notdir $*).d $(1) -c -o $@ $(call sp_convert_path,$<)
sp_compile_c = $(GLOBAL_QUIET_CC) $(GLOBAL_MKDIR) $(dir $@); $(GLOBAL_CC) -MMD -MP -MF $(dir $@)/$(notdir $*).d $(1) -c -o $@ $(call sp_convert_path,$<)
sp_compile_cpp = $(GLOBAL_QUIET_CPP) $(GLOBAL_MKDIR) $(dir $@); $(GLOBAL_CPP) -MMD -MP -MF $(dir $@)/$(notdir $*).d $(1) -c -o $@ $(call sp_convert_path,$<)
sp_compile_mm = $(GLOBAL_QUIET_CPP) $(GLOBAL_MKDIR) $(dir $@); $(GLOBAL_CPP) -MMD -MP -MF $(dir $@)/$(notdir $*).d $(1) -fobjc-arc -c -o $@ $(call sp_convert_path,$<)

sp_toolkit_source_list = $(realpath\
	$(foreach dir,$(1),$(shell find $(GLOBAL_ROOT)/$(dir) \( -name "*.c" -or -name "*.cpp" \)))\
	$(addprefix $(GLOBAL_ROOT)/,$(filter-out %.mm,$(2)))\
	$(if $(BUILD_OBJC),\
		$(foreach dir,$(1),$(shell find $(GLOBAL_ROOT)/$(dir) -name '*.mm'))\
		$(addprefix $(GLOBAL_ROOT)/,$(filter %.mm,$(2)))\
	)\
)

sp_toolkit_include_list = $(realpath\
	$(foreach dir,$(1),$(shell find $(GLOBAL_ROOT)/$(dir) -type d)) \
	$(addprefix $(GLOBAL_ROOT)/,$(2))\
)

sp_toolkit_object_list = $(abspath $(addprefix $(1),$(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(patsubst %.mm,%.o,$(2))))))

sp_toolkit_prefix_files_list = \
	$(abspath $(addprefix $(1)/include,$(realpath $(addprefix $(GLOBAL_ROOT)/,$(2)))))

sp_toolkit_include_flags = \
	$(addprefix -I,$(sort $(dir $(1)))) $(addprefix -I,$(2))

sp_local_source_list = \
	$(foreach dir,$(filter /%,$(1)),$(shell find $(dir) -name '*.cpp')) \
	$(foreach dir,$(filter /%,$(1)),$(shell find $(dir) -name '*.c')) \
	$(filter /%,$(filter-out %.mm,$(2))) \
	$(foreach dir,$(filter-out /%,$(1)),$(shell find $(LOCAL_ROOT)/$(dir) -name '*.cpp')) \
	$(foreach dir,$(filter-out /%,$(1)),$(shell find $(LOCAL_ROOT)/$(dir) -name '*.c')) \
	$(addprefix $(LOCAL_ROOT)/,$(filter-out /%,$(filter-out %.mm,$(2)))) \
	$(if $(BUILD_OBJC),\
		$(foreach dir,$(filter /%,$(1)),$(shell find $(dir) -name '*.mm'))\
		$(foreach dir,$(filter-out /%,$(1)),$(shell find $(LOCAL_ROOT)/$(dir) -name '*.mm'))\
		$(filter /%,$(filter %.mm,$(2)))\
		$(addprefix $(LOCAL_ROOT)/,$(filter-out /%,$(filter %.mm,$(2))))\
	) \

sp_local_include_list = \
	$(foreach dir,$(filter /%,$(1)),$(shell find $(dir) -type d)) \
	$(filter /%,$(2)) \
	$(foreach dir,$(filter-out /%,$(1)),$(shell find $(LOCAL_ROOT)/$(dir) -type d)) \
	$(addprefix $(LOCAL_ROOT)/,$(filter-out /%,$(2))) \
	$(3)

sp_local_object_list = \
	$(addprefix $(1),$(patsubst %.mm,%.o,$(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(realpath $(2))))))
