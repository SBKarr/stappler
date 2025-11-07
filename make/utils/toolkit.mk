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

$(TOOLKIT_NAME)_LIBS := -L$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH) $(OSTYPE_$(TOOLKIT_NAME)_LIBS) $(LDFLAGS)
$(TOOLKIT_NAME)_SRCS := $(call sp_toolkit_source_list, $($(TOOLKIT_NAME)_SRCS_DIRS), $($(TOOLKIT_NAME)_SRCS_OBJS))\
	$(call sp_toolkit_source_list_abs, $($(TOOLKIT_NAME)_SRCS_DIRS_ABS), $($(TOOLKIT_NAME)_SRCS_OBJS_ABS))
$(TOOLKIT_NAME)_INCLUDES := $(call sp_toolkit_include_list, $($(TOOLKIT_NAME)_INCLUDES_DIRS), $($(TOOLKIT_NAME)_INCLUDES_OBJS))

$(TOOLKIT_NAME)_H_GCH := $(call sp_toolkit_prefix_files_list,$($(TOOLKIT_NAME)_OUTPUT_DIR), $($(TOOLKIT_NAME)_PRECOMPILED_HEADERS))
$(TOOLKIT_NAME)_GCH := $(addsuffix .gch,$($(TOOLKIT_NAME)_H_GCH))

$(TOOLKIT_NAME)_OBJS := $(call sp_toolkit_object_list, $($(TOOLKIT_NAME)_OUTPUT_DIR), $($(TOOLKIT_NAME)_SRCS))

$(TOOLKIT_NAME)_INPUT_CFLAGS := $(call sp_toolkit_include_flags,$($(TOOLKIT_NAME)_GCH),$($(TOOLKIT_NAME)_INCLUDES))

$(TOOLKIT_NAME)_CXXFLAGS := $(GLOBAL_CXXFLAGS) $($(TOOLKIT_NAME)_FLAGS) $($(TOOLKIT_NAME)_INPUT_CFLAGS)
$(TOOLKIT_NAME)_CFLAGS := $(GLOBAL_CFLAGS) $($(TOOLKIT_NAME)_FLAGS) $($(TOOLKIT_NAME)_INPUT_CFLAGS)

$(TOOLKIT_NAME)_COMPILATION_DATABASE := $($(TOOLKIT_NAME)_OUTPUT_DIR)/compile_commands.json

COUNTER_WORDS := $(words $($(TOOLKIT_NAME)_GCH) $($(TOOLKIT_NAME)_OBJS))
COUNTER_NAME := $(TOOLKIT_TITLE)

include $(GLOBAL_ROOT)/make/utils/counter.mk
$(foreach obj,$($(TOOLKIT_NAME)_GCH) $($(TOOLKIT_NAME)_OBJS),$(eval $(call counter_template,$(obj))))

# include dependencies
-include $(addsuffix .d,$($(TOOLKIT_NAME)_OBJS))
-include $(addsuffix .d,$($(TOOLKIT_NAME)_GCH))

# $(1) is a parameter to substitute. The $$ will expand as $.

define $(TOOLKIT_NAME)_include_rule
$(1): $(subst $(2)/,/,$(1))
	$$(call sp_copy_header,$(1),$(2))
endef

define $(TOOLKIT_NAME)_gch_rule
$(1): $(patsubst %.h.gch,%.h,$(1)) $(call sp_make_dep,$(1))
	$$(call sp_compile_gch,$(2))
endef

define $(TOOLKIT_NAME)_c_rule
$(2).json: $(1) $$(LOCAL_MAKEFILE)
	@$(GLOBAL_MKDIR) $$(dir $$@)
	@echo "{" > $$@
	@echo '"directory":"'$$(call sp_cdb_convert_cmd,$$(BUILD_WORKDIR))'",' >> $$@
	@echo '"file":"'$$(call sp_cdb_convert_cmd,$(1))'",' >> $$@
	@echo '"output":"'$$(call sp_cdb_convert_cmd,$(2))'",' >> $$@
	@echo '"arguments":[$$(call sp_cdb_split_arguments_cmd,$$(GLOBAL_CC),$$(call sp_compile_command,,$$(OSTYPE_C_FILE),$(3),$(1),$(2)))]' >> $$@
	@echo "}," >> $$@
	@echo [Compilation database entry]: $(notdir $(1))

$(2): $(1) $$($$(TOOLKIT_NAME)_H_GCH) $$($$(TOOLKIT_NAME)_GCH) $(call sp_make_dep,$(2)) | $(2).json
	$$(call sp_compile_c,$(3))
endef

define $(TOOLKIT_NAME)_cpp_rule
$(2).json: $(1) $$(LOCAL_MAKEFILE)
	@$(GLOBAL_MKDIR) $$(dir $$@)
	@echo "{" > $$@
	@echo '"directory":"'$$(call sp_cdb_convert_cmd,$$(BUILD_WORKDIR))'",' >> $$@
	@echo '"file":"'$$(call sp_cdb_convert_cmd,$(1))'",' >> $$@
	@echo '"output":"'$$(call sp_cdb_convert_cmd,$(2))'",' >> $$@
	@echo '"arguments":[$$(call sp_cdb_split_arguments_cmd,$$(GLOBAL_CPP),$$(call sp_compile_command,,$$(OSTYPE_CPP_FILE),$(3),$(1),$(2)))]' >> $$@
	@echo "}," >> $$@
	@echo [Compilation database entry]: $(notdir $(1))

$(2): $(1) $$($$(TOOLKIT_NAME)_H_GCH) $$($$(TOOLKIT_NAME)_GCH) $(call sp_make_dep,$(2)) | $(2).json
	$$(call sp_compile_cpp,$(3))
endef

define $(TOOLKIT_NAME)_mm_rule
$(2).json: $(1) $$(LOCAL_MAKEFILE)
	@$(GLOBAL_MKDIR) $$(dir $$@)
	@echo "{" > $$@
	@echo '"directory":"'$$(call sp_cdb_convert_cmd,$$(BUILD_WORKDIR))'",' >> $$@
	@echo '"file":"'$$(call sp_cdb_convert_cmd,$(1))'",' >> $$@
	@echo '"output":"'$$(call sp_cdb_convert_cmd,$(2))'",' >> $$@
	@echo '"arguments":[$$(call sp_cdb_split_arguments_cmd,$$(GLOBAL_CPP),$$(call sp_compile_command,,$$(OSTYPE_MM_FILE),$(3),$(1),$(2)))]' >> $$@
	@echo "}," >> $$@
	@echo [Compilation database entry]: $(notdir $(1))

$(2): $(1) $$($$(TOOLKIT_NAME)_H_GCH) $$($$(TOOLKIT_NAME)_GCH) $(call sp_make_dep,$(2)) | $(2).json
	$$(call sp_compile_mm,$(3))
endef

$(foreach target,$(patsubst %.h.gch,%.h,$($(TOOLKIT_NAME)_GCH)),$(eval $(call $(TOOLKIT_NAME)_include_rule,$(target),$($(TOOLKIT_NAME)_OUTPUT_DIR)/include)))

$(foreach target,$($(TOOLKIT_NAME)_GCH),$(eval $(call $(TOOLKIT_NAME)_gch_rule,$(target),$($(TOOLKIT_NAME)_CXXFLAGS))))

$(foreach source,\
	$(filter %.c,$($(TOOLKIT_NAME)_SRCS)),\
	$(eval $(call $(TOOLKIT_NAME)_c_rule,\
		$(source),\
		$(abspath $(addprefix $($(TOOLKIT_NAME)_OUTPUT_DIR)/,$(addsuffix .o,$(notdir $(source))))), \
		$($(TOOLKIT_NAME)_CFLAGS))))

$(foreach source,\
	$(filter %.cpp,$($(TOOLKIT_NAME)_SRCS)),\
	$(eval $(call $(TOOLKIT_NAME)_cpp_rule,\
		$(source),\
		$(abspath $(addprefix $($(TOOLKIT_NAME)_OUTPUT_DIR)/,$(addsuffix .o,$(notdir $(source))))), \
		$($(TOOLKIT_NAME)_CXXFLAGS))))

$(foreach source,\
	$(filter %.mm,$($(TOOLKIT_NAME)_SRCS)),\
	$(eval $(call $(TOOLKIT_NAME)_mm_rule,\
		$(source),\
		$(abspath $(addprefix $($(TOOLKIT_NAME)_OUTPUT_DIR)/,$(addsuffix .o,$(notdir $(source))))), \
		$($(TOOLKIT_NAME)_CXXFLAGS))))

$(TOOLKIT_NAME)_CDB_TARGET_JSON := $(addsuffix .json,$($(TOOLKIT_NAME)_OBJS))

$(eval $(call BUILD_cdb,$($(TOOLKIT_NAME)_COMPILATION_DATABASE),$($(TOOLKIT_NAME)_CDB_TARGET_JSON)))

ifeq ($(UNAME),Msys)
.INTERMEDIATE: $(subst /,_,$($(TOOLKIT_NAME)_GCH) $($(TOOLKIT_NAME)_OBJS))

$(subst /,_,$($(TOOLKIT_NAME)_GCH) $($(TOOLKIT_NAME)_OBJS)):
	@touch $@

endif
