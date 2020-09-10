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

COUNTER_WORDS := $(words $($(TOOLKIT_NAME)_GCH) $($(TOOLKIT_NAME)_OBJS))
COUNTER_NAME := $(TOOLKIT_TITLE)

include $(GLOBAL_ROOT)/make/utils/counter.mk
$(foreach obj,$($(TOOLKIT_NAME)_GCH) $($(TOOLKIT_NAME)_OBJS),$(eval $(call counter_template,$(obj))))

# include dependencies
-include $(patsubst %.o,%.o.d,$($(TOOLKIT_NAME)_OBJS))
-include $(patsubst %.h.gch,%.h.gch.d,$($(TOOLKIT_NAME)_GCH))

# $(1) is a parameter to substitute. The $$ will expand as $.
define $(TOOLKIT_NAME)_gch_rule
$(1): $(patsubst %.h.gch,%.h,$(1)) $(call sp_make_dep,$(1))
	$$(call sp_compile_gch,$(2))
endef

define $(TOOLKIT_NAME)_c_rule
$(abspath $(addprefix $(2),$(patsubst %.c,%.o,$(1)))): $(1) $$($$(TOOLKIT_NAME)_H_GCH) $$($$(TOOLKIT_NAME)_GCH) \
		$(call sp_make_dep,$(abspath $(addprefix $(2),$(patsubst %.c,%.o,$(1)))))
	$$(call sp_compile_c,$(3))
endef

define $(TOOLKIT_NAME)_cpp_rule
$(abspath $(addprefix $(2),$(patsubst %.cpp,%.o,$(1)))): $(1) $$($$(TOOLKIT_NAME)_H_GCH) $$($$(TOOLKIT_NAME)_GCH) \
		$(call sp_make_dep,$(abspath $(addprefix $(2),$(patsubst %.cpp,%.o,$(1)))))
	$$(call sp_compile_cpp,$(3))
endef

define $(TOOLKIT_NAME)_mm_rule
$(abspath $(addprefix $(2),$(patsubst %.mm,%.o,$(1)))): $(1) $$($$(TOOLKIT_NAME)_H_GCH) $$($$(TOOLKIT_NAME)_GCH) \
		$(call sp_make_dep,$(abspath $(addprefix $(2),$(patsubst %.mm,%.o,$(1)))))
	$$(call sp_compile_mm,$(3))
endef

$(foreach target,$($(TOOLKIT_NAME)_GCH),$(eval $(call $(TOOLKIT_NAME)_gch_rule,$(target),$($(TOOLKIT_NAME)_CXXFLAGS))))

$(foreach target,\
	$(filter %.c,$($(TOOLKIT_NAME)_SRCS)),\
	$(eval $(call $(TOOLKIT_NAME)_c_rule,$(target),$($(TOOLKIT_NAME)_OUTPUT_DIR),$($(TOOLKIT_NAME)_CFLAGS))))

$(foreach target,\
	$(filter %.cpp,$($(TOOLKIT_NAME)_SRCS)),\
	$(eval $(call $(TOOLKIT_NAME)_cpp_rule,$(target),$($(TOOLKIT_NAME)_OUTPUT_DIR),$($(TOOLKIT_NAME)_CXXFLAGS))))

$(foreach target,\
	$(filter %.mm,$($(TOOLKIT_NAME)_SRCS)),\
	$(eval $(call $(TOOLKIT_NAME)_mm_rule,$(target),$($(TOOLKIT_NAME)_OUTPUT_DIR),$($(TOOLKIT_NAME)_CXXFLAGS))))

ifeq ($(UNAME),Msys)
.INTERMEDIATE: $(subst /,_,$($(TOOLKIT_NAME)_GCH) $($(TOOLKIT_NAME)_OBJS))

$(subst /,_,$($(TOOLKIT_NAME)_GCH) $($(TOOLKIT_NAME)_OBJS)):
	@touch $@

endif