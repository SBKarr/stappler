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

.DEFAULT_GOAL := all
IS_LOCAL_BUILD := 1

BUILD_OUTDIR := $(LOCAL_OUTDIR)/host

LOCAL_INSTALL_DIR ?= $(LOCAL_OUTDIR)/host

GLOBAL_ROOT := $(STAPPLER_ROOT)
GLOBAL_OUTPUT := $(BUILD_OUTDIR)

include $(GLOBAL_ROOT)/make/utils/compiler.mk

BUILD_OUTDIR := $(BUILD_OUTDIR)/$(GLOBAL_CC)/$(if $(RELEASE),release,debug)

ifdef LOCAL_LIBRARY
ifeq ($(LOCAL_TOOLKIT),none)
BUILD_STATIC_LIBRARY := $(BUILD_OUTDIR)/$(LOCAL_LIBRARY).a
BUILD_INSTALL_STATIC_LIBRARY := $(LOCAL_INSTALL_DIR)/$(LOCAL_LIBRARY).a
else
BUILD_SHARED_LIBRARY := $(BUILD_OUTDIR)/$(LOCAL_LIBRARY).so
BUILD_INSTALL_SHARED_LIBRARY := $(LOCAL_INSTALL_DIR)/$(LOCAL_LIBRARY).so
endif
endif

ifdef LOCAL_EXECUTABLE
BUILD_EXECUTABLE := $(BUILD_OUTDIR)/$(LOCAL_EXECUTABLE)
BUILD_INSTALL_EXECUTABLE := $(LOCAL_INSTALL_DIR)/$(LOCAL_EXECUTABLE)
endif

include $(GLOBAL_ROOT)/make/utils/external.mk

all: $(BUILD_SHARED_LIBRARY) $(BUILD_EXECUTABLE) $(BUILD_STATIC_LIBRARY)

ifdef LOCAL_FORCE_INSTALL
all: install
endif

$(BUILD_EXECUTABLE) : $(BUILD_OBJS) $(BUILD_MAIN_OBJ)
	$(GLOBAL_QUIET_LINK) $(GLOBAL_CPP) $(BUILD_OBJS) $(BUILD_MAIN_OBJ) $(BUILD_LIBS) $(TOOLKIT_LIB_FLAGS) $(OSTYPE_EXEC_FLAGS) -o $(BUILD_EXECUTABLE)

$(BUILD_SHARED_LIBRARY): $(BUILD_OBJS)
	$(GLOBAL_QUIET_LINK) $(GLOBAL_CPP) -shared  $(BUILD_OBJS) $(BUILD_LIBS) $(TOOLKIT_LIB_FLAGS) $(OSTYPE_EXEC_FLAGS) -o $(BUILD_SHARED_LIBRARY)

$(BUILD_STATIC_LIBRARY): $(BUILD_OBJS)
	$(GLOBAL_QUIET_LINK) $(GLOBAL_AR) $(BUILD_STATIC_LIBRARY) $(BUILD_OBJS)

$(BUILD_OUTDIR)/%.o: /%.cpp $(TOOLKIT_H_GCH) $(TOOLKIT_GCH)
	$(call sp_compile_cpp,$(BUILD_CXXFLAGS))

$(BUILD_OUTDIR)/%.o: /%.mm $(TOOLKIT_H_GCH) $(TOOLKIT_GCH)
	$(call sp_compile_mm,$(BUILD_CXXFLAGS))

$(BUILD_OUTDIR)/%.o: /%.c $(TOOLKIT_H_GCH) $(TOOLKIT_GCH)
	$(call sp_compile_c,$(BUILD_CFLAGS))

$(BUILD_INSTALL_SHARED_LIBRARY): $(BUILD_SHARED_LIBRARY)
	@$(GLOBAL_MKDIR) $(dir $(BUILD_INSTALL_SHARED_LIBRARY))
	$(GLOBAL_CP) $(BUILD_SHARED_LIBRARY) $(BUILD_INSTALL_SHARED_LIBRARY)

$(BUILD_INSTALL_EXECUTABLE): $(BUILD_EXECUTABLE)
	@$(GLOBAL_MKDIR) $(dir $(BUILD_INSTALL_EXECUTABLE))
	$(GLOBAL_CP) $(BUILD_EXECUTABLE) $(BUILD_INSTALL_EXECUTABLE)

install: $(BUILD_INSTALL_SHARED_LIBRARY) $(BUILD_INSTALL_EXECUTABLE) $(BUILD_INSTALL_STATIC_LIBRARY)

.PHONY: all install

ifeq ($(UNAME),Msys)

define LOCAL_make_dep_rule
$(1): $(call sp_make_dep,$(1))
endef

$(foreach target,$(BUILD_LOCAL_OBJS),$(eval $(call LOCAL_make_dep_rule,$(target))))

.INTERMEDIATE: $(subst /,_,$(BUILD_LOCAL_OBJS))

$(subst /,_,$(BUILD_LOCAL_OBJS)):
	@touch $@

endif