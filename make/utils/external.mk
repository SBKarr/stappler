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

BUILD_SRCS := $(call sp_local_source_list,$(LOCAL_SRCS_DIRS),$(LOCAL_SRCS_OBJS))
BUILD_INCLUDES := $(call sp_local_include_list,$(LOCAL_INCLUDES_DIRS),$(LOCAL_INCLUDES_OBJS),$(LOCAL_ABSOLUTE_INCLUDES))
BUILD_OBJS := $(abspath  $(call sp_local_object_list,$(BUILD_OUTDIR),$(BUILD_SRCS)))

ifdef LOCAL_MAIN
BUILD_MAIN_SRC := $(realpath $(addprefix $(LOCAL_ROOT)/,$(LOCAL_MAIN)))
BUILD_MAIN_OBJ := $(abspath $(addprefix $(BUILD_OUTDIR)/,$(addsuffix .o,$(BUILD_MAIN_SRC))))
else
BUILD_MAIN_OBJ := 
endif



ifdef LOCAL_TOOLKIT_OUTPUT
TOOLKIT_OUTPUT := $(LOCAL_TOOLKIT_OUTPUT)
endif


ifdef LOCAL_TOOLKIT_EXTERNAL
LOCAL_TOOLKIT := none
endif

include $(GLOBAL_ROOT)/make/build.mk

LOCAL_TOOLKIT ?= material

ifdef LOCAL_TOOLKIT_EXTERNAL
TOOLKIT_SRCS := $($(TOOLKIT_NAME)_SRCS)
TOOLKIT_OBJS := $($(TOOLKIT_NAME)_OBJS)
TOOLKIT_CFLAGS := $($(TOOLKIT_NAME)_CFLAGS)
TOOLKIT_CXXFLAGS := $($(TOOLKIT_NAME)_CXXFLAGS)
TOOLKIT_LIBS := $($(TOOLKIT_NAME)_LIBS)
TOOLKIT_LIB_FLAGS := $($(TOOLKIT_NAME)_LDFLAGS)
TOOLKIT_H_GCH := $($(TOOLKIT_NAME)_H_GCH)
TOOLKIT_GCH := $($(TOOLKIT_NAME)_GCH)
else
ifeq ($(LOCAL_TOOLKIT),material)
TOOLKIT_SRCS := $(MATERIAL_SRCS) $(STAPPLER_SRCS)
TOOLKIT_OBJS := $(STAPPLER_OBJS) $(MATERIAL_OBJS)
TOOLKIT_CFLAGS := $(MATERIAL_CFLAGS)
TOOLKIT_CXXFLAGS := $(MATERIAL_CXXFLAGS)
TOOLKIT_LIBS := $(STAPPLER_LIBS)
TOOLKIT_LIB_FLAGS := $(OSTYPE_LDFLAGS)
TOOLKIT_H_GCH := $(MATERIAL_H_GCH) $(STAPPLER_H_GCH)
TOOLKIT_GCH := $(MATERIAL_GCH) $(STAPPLER_GCH)
endif
ifeq ($(LOCAL_TOOLKIT),stappler)
TOOLKIT_SRCS := $(STAPPLER_SRCS)
TOOLKIT_OBJS := $(STAPPLER_OBJS)
TOOLKIT_CFLAGS := $(STAPPLER_CFLAGS)
TOOLKIT_CXXFLAGS := $(STAPPLER_CXXFLAGS)
TOOLKIT_LIBS := $(STAPPLER_LIBS)
TOOLKIT_LIB_FLAGS := $(OSTYPE_LDFLAGS)
TOOLKIT_H_GCH := $(STAPPLER_H_GCH)
TOOLKIT_GCH := $(STAPPLER_GCH)
endif
ifeq ($(LOCAL_TOOLKIT),cli)
TOOLKIT_SRCS := $(CLI_SRCS)
TOOLKIT_OBJS := $(CLI_OBJS)
TOOLKIT_CFLAGS := $(CLI_CFLAGS)
TOOLKIT_CXXFLAGS := $(CLI_CXXFLAGS)
TOOLKIT_LIBS := $(CLI_LIBS)
TOOLKIT_LIB_FLAGS := $(OSTYPE_LDFLAGS)
TOOLKIT_H_GCH := $(CLI_H_GCH)
TOOLKIT_GCH := $(CLI_GCH)
endif
ifeq ($(LOCAL_TOOLKIT),common)
TOOLKIT_SRCS := $(COMMON_SRCS)
TOOLKIT_OBJS := $(COMMON_OBJS)
TOOLKIT_CFLAGS := $(COMMON_CFLAGS)
TOOLKIT_CXXFLAGS := $(COMMON_CXXFLAGS)
TOOLKIT_LIBS := $(COMMON_LIBS)
TOOLKIT_LIB_FLAGS := $(OSTYPE_LDFLAGS)
TOOLKIT_H_GCH := $(COMMON_H_GCH)
TOOLKIT_GCH := $(COMMON_GCH)
endif
ifeq ($(LOCAL_TOOLKIT),serenity)
TOOLKIT_CFLAGS := $(SERENITY_CFLAGS)
TOOLKIT_CXXFLAGS := $(SERENITY_CXXFLAGS)
TOOLKIT_LIBS := $(SERENITY_LIBS)
TOOLKIT_LIB_FLAGS := 
TOOLKIT_H_GCH := $(SERENITY_H_GCH)
TOOLKIT_GCH := $(SERENITY_GCH)
endif
ifeq ($(LOCAL_TOOLKIT),stellator)
TOOLKIT_SRCS := $(STELLATOR_SRCS)
TOOLKIT_OBJS := $(STELLATOR_OBJS)
TOOLKIT_CFLAGS := $(STELLATOR_CFLAGS)
TOOLKIT_CXXFLAGS := $(STELLATOR_CXXFLAGS)
TOOLKIT_LIBS := $(STELLATOR_LIBS)
TOOLKIT_LIB_FLAGS := $(OSTYPE_LDFLAGS)
TOOLKIT_H_GCH := $(STELLATOR_H_GCH)
TOOLKIT_GCH := $(STELLATOR_GCH)
endif
ifeq ($(LOCAL_TOOLKIT),none)
TOOLKIT_CFLAGS := $(COMMON_CFLAGS)
TOOLKIT_CXXFLAGS := $(COMMON_CXXFLAGS)
TOOLKIT_LIBS := 
TOOLKIT_LIB_FLAGS := 
TOOLKIT_H_GCH :=
TOOLKIT_GCH :=
endif
endif

BUILD_CFLAGS += $(LOCAL_CFLAGS) $(TOOLKIT_CFLAGS)
BUILD_CXXFLAGS += $(LOCAL_CFLAGS) $(LOCAL_CXXFLAGS) $(TOOLKIT_CXXFLAGS)

# Progress counter
BUILD_COUNTER := 0
BUILD_WORDS := $(words $(BUILD_OBJS) $(BUILD_MAIN_OBJ))

define BUILD_template =
$(eval BUILD_COUNTER=$(shell echo $$(($(BUILD_COUNTER)+1))))
$(1):BUILD_CURRENT_COUNTER:=$(BUILD_COUNTER)
$(1):BUILD_FILES_COUNTER := $(BUILD_WORDS)
ifdef LOCAL_EXECUTABLE
$(1):BUILD_LIBRARY := $(notdir $(LOCAL_EXECUTABLE))
else
$(1):BUILD_LIBRARY := $(notdir $(LOCAL_LIBRARY))
endif
endef

$(foreach obj,$(BUILD_OBJS) $(BUILD_MAIN_OBJ),$(eval $(call BUILD_template,$(obj))))

BUILD_LOCAL_SRCS := $(BUILD_SRCS) $(BUILD_MAIN_SRCS)
BUILD_LOCAL_OBJS := $(BUILD_OBJS) $(BUILD_MAIN_OBJ)

BUILD_OBJS += $(TOOLKIT_OBJS)

BUILD_CFLAGS += $(addprefix -I,$(BUILD_INCLUDES))
BUILD_CXXFLAGS += $(addprefix -I,$(BUILD_INCLUDES))

BUILD_LIBS := $(foreach lib,$(LOCAL_LIBS),-L$(dir $(lib)) -l:$(notdir $(lib))) $(TOOLKIT_LIBS) $(LDFLAGS)

BUILD_COMPILATION_DATABASE := ./compile_commands.json

define BUILD_c_rule
$(2).json: $(1) $$(LOCAL_MAKEFILE)
	@$(GLOBAL_MKDIR) $$(dir $$@)
	@echo "{" > $$@
	@echo '"directory":"'$$(call sp_cdb_convert_cmd,$$(BUILD_WORKDIR))'",' >> $$@
	@echo '"file":"'$$(call sp_cdb_convert_cmd,$(1))'",' >> $$@
	@echo '"output":"'$$(call sp_cdb_convert_cmd,$(2))'",' >> $$@
	@echo '"arguments":[$$(call sp_cdb_split_arguments_cmd,$$(GLOBAL_CC),$$(call sp_compile_command,,$$(OSTYPE_C_FILE),$(3),$(1),$(2)))]' >> $$@
	@echo "}," >> $$@
	@echo [Compilation database entry]: $(notdir $(1))

$(2): $(1) $$(TOOLKIT_H_GCH) $$(TOOLKIT_GCH) $(call sp_make_dep,$(2)) | $(2).json $$(BUILD_COMPILATION_DATABASE)
	$$(call sp_compile_c,$(3))
endef

define BUILD_cpp_rule
$(2).json: $(1) $$(LOCAL_MAKEFILE)
	@$(GLOBAL_MKDIR) $$(dir $$@)
	@echo "{" > $$@
	@echo '"directory":"'$$(call sp_cdb_convert_cmd,$$(BUILD_WORKDIR))'",' >> $$@
	@echo '"file":"'$$(call sp_cdb_convert_cmd,$(1))'",' >> $$@
	@echo '"output":"'$$(call sp_cdb_convert_cmd,$(2))'",' >> $$@
	@echo '"arguments":[$$(call sp_cdb_split_arguments_cmd,$$(GLOBAL_CPP),$$(call sp_compile_command,,$$(OSTYPE_CPP_FILE),$(3),$(1),$(2)))]' >> $$@
	@echo "}," >> $$@
	@echo [Compilation database entry]: $(notdir $(1))

$(2): $(1) $$(TOOLKIT_H_GCH) $$(TOOLKIT_GCH) $(call sp_make_dep,$(2)) | $(2).json $$(BUILD_COMPILATION_DATABASE)
	$$(call sp_compile_cpp,$(3))
endef

define BUILD_mm_rule
$(2).json: $(1) $$(LOCAL_MAKEFILE)
	@$(GLOBAL_MKDIR) $$(dir $$@)
	@echo "{" > $$@
	@echo '"directory":"'$$(call sp_cdb_convert_cmd,$$(BUILD_WORKDIR))'",' >> $$@
	@echo '"file":"'$$(call sp_cdb_convert_cmd,$(1))'",' >> $$@
	@echo '"output":"'$$(call sp_cdb_convert_cmd,$(2))'",' >> $$@
	@echo '"arguments":[$$(call sp_cdb_split_arguments_cmd,$$(GLOBAL_CPP),$$(call sp_compile_command,,$$(OSTYPE_MM_FILE),$(3),$(1),$(2)))]' >> $$@
	@echo "}," >> $$@
	@echo [Compilation database entry]: $(notdir $(1))

$(2): $(1) $$(TOOLKIT_H_GCH) $$(TOOLKIT_GCH) $(call sp_make_dep,$(2)) | $(2).json $$(BUILD_COMPILATION_DATABASE)
	$$(call sp_compile_mm,$(3))
endef

$(foreach source,\
	$(filter %.c,$(BUILD_LOCAL_SRCS)),\
	$(eval $(call BUILD_c_rule,\
		$(realpath $(source)),\
		$(abspath $(addprefix $(BUILD_OUTDIR)/,$(addsuffix .o,$(notdir $(source))))), \
		$(BUILD_CFLAGS))))

$(foreach source,\
	$(filter %.cpp,$(BUILD_LOCAL_SRCS)),\
	$(eval $(call BUILD_cpp_rule,\
		$(realpath $(source)),\
		$(abspath $(addprefix $(BUILD_OUTDIR)/,$(addsuffix .o,$(notdir $(source))))), \
		$(BUILD_CXXFLAGS))))

$(foreach source,\
	$(filter %.mm,$(BUILD_LOCAL_SRCS)),\
	$(eval $(call BUILD_mm_rule,\
		$(realpath $(source)),\
		$(abspath $(addprefix $(BUILD_OUTDIR)/,$(addsuffix .o,$(notdir $(source))))), \
		$(BUILD_CXXFLAGS))))

BUILD_CDB_TARGET_JSON := $(addsuffix .json,$(BUILD_OBJS))

$(eval $(call BUILD_cdb,$(BUILD_COMPILATION_DATABASE),$(BUILD_CDB_TARGET_JSON)))

-include $(addsuffix .d,$(BUILD_OBJS) $(BUILD_MAIN_OBJ))
