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

ifndef APACHE_INCLUDE
APACHE_INCLUDE := /usr/local/include/apache
endif

SERENITY_OUTPUT_DIR = $(TOOLKIT_OUTPUT)/serenity
SERENITY_OUTPUT = $(TOOLKIT_OUTPUT)/mod_serenity.so
SERENITY_SRCS_DIRS += \
	common \
	serenity/apr \
	serenity/src \
	serenity/spug \

SERENITY_PRECOMPILED_HEADERS += \
	serenity/src/core/Define.h \
	common/core/SPCore.h \
	common/core/SPCommon.h

SERENITY_FLAGS := -DSPAPR

SERENITY_SRCS_OBJS += \
	serenity/gen/__Version.cpp \
	serenity/gen/__Virtual.cpp

SERENITY_INCLUDES_DIRS += \
	common \
	serenity/src \
	serenity/apr \
	serenity/spug \

SERENITY_INCLUDES_OBJS += \
	serenity/ext/cityhash

SERENITY_LIBS += -L$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH) $(OSTYPE_SERENITY_LIBS) -lpq -lidn

SERENITY_SRCS := \
	$(foreach dir,$(SERENITY_SRCS_DIRS),$(shell find $(GLOBAL_ROOT)/$(dir) -name '*.cpp')) \
	$(addprefix $(GLOBAL_ROOT)/,$(SERENITY_SRCS_OBJS))
	
SERENITY_FILE_SRCS := \
	$(foreach dir,$(SERENITY_SRCS_DIRS),$(shell find $(GLOBAL_ROOT)/$(dir) -name '*.c*')) \
	$(addprefix $(GLOBAL_ROOT)/,serenity/gen/__Virtual.cpp)

SERENITY_INCLUDES := \
	$(foreach dir,$(SERENITY_INCLUDES_DIRS),$(shell find $(GLOBAL_ROOT)/$(dir) -type d)) \
	$(addprefix $(GLOBAL_ROOT)/,$(SERENITY_INCLUDES_OBJS)) \
	$(APACHE_INCLUDE) \
	$(GLOBAL_ROOT)/$(OSTYPE_INCLUDE)

SERENITY_VIRTUAL_SRCS := \
	$(shell find $(GLOBAL_ROOT)/serenity/virtual/js -name '*.js') \
	$(shell find $(GLOBAL_ROOT)/serenity/virtual/css -name '*.css') \
	$(shell find $(GLOBAL_ROOT)/serenity/virtual/html -name '*.html') \
	$(shell find $(GLOBAL_ROOT)/serenity/virtual/html -name '*.pug')

SERENITY_GCH := $(addprefix $(GLOBAL_ROOT)/,$(SERENITY_PRECOMPILED_HEADERS))
SERENITY_H_GCH := $(patsubst $(GLOBAL_ROOT)/%,$(SERENITY_OUTPUT_DIR)/include/%,$(SERENITY_GCH))
SERENITY_GCH := $(addsuffix .gch,$(SERENITY_H_GCH))

SERENITY_OBJS := $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(patsubst $(GLOBAL_ROOT)/%,$(SERENITY_OUTPUT_DIR)/%,$(SERENITY_SRCS))))
SERENITY_DIRS := $(sort $(dir $(SERENITY_OBJS))) $(sort $(dir $(SERENITY_GCH)))

SERENITY_INPUT_CFLAGS := $(addprefix -I,$(sort $(dir $(SERENITY_GCH)))) $(addprefix -I,$(SERENITY_INCLUDES))

SERENITY_CXXFLAGS := $(GLOBAL_CXXFLAGS) $(SERENITY_FLAGS) $(SERENITY_INPUT_CFLAGS)
SERENITY_CFLAGS := $(GLOBAL_CFLAGS) $(SERENITY_FLAGS) $(SERENITY_INPUT_CFLAGS)

# Progress counter
SERENITY_COUNTER := 0
SERENITY_WORDS := $(words $(SERENITY_GCH) $(SERENITY_OBJS))

define SERENITY_template =
$(eval SERENITY_COUNTER=$(shell echo $$(($(SERENITY_COUNTER)+1))))
$(1):BUILD_CURRENT_COUNTER:=$(SERENITY_COUNTER)
$(1):BUILD_FILES_COUNTER:=$(SERENITY_WORDS)
$(1):BUILD_LIBRARY := serenity
endef

STAPPLER_MAKE_ROOT ?= $(realpath $(GLOBAL_ROOT))
STAPPLER_REPO_TAG := $(shell $(STAPPLER_MAKE_ROOT)/make-version.sh $(STAPPLER_MAKE_ROOT))

$(foreach obj,$(SERENITY_GCH) $(SERENITY_OBJS),$(eval $(call SERENITY_template,$(obj))))

-include $(patsubst %.o,%.d,$(SERENITY_OBJS))
-include $(patsubst %.gch,%.d,$(SERENITY_GCH))

$(SERENITY_OUTPUT_DIR)/include/%.h : $(GLOBAL_ROOT)/%.h
	@$(GLOBAL_MKDIR) $(dir $(SERENITY_OUTPUT_DIR)/include/$*.h)
	@cp -f $< $(SERENITY_OUTPUT_DIR)/include/$*.h

$(SERENITY_OUTPUT_DIR)/include/%.h.gch: $(SERENITY_OUTPUT_DIR)/include/%.h
	$(call sp_compile_gch,$(SERENITY_CXXFLAGS))

$(GLOBAL_ROOT)/serenity/gen/__Virtual.cpp: $(SERENITY_VIRTUAL_SRCS)
	@mkdir -p $(GLOBAL_ROOT)/serenity/gen
	@echo '// Files for virtual filesystem\n// Autogenerated by makefile (serenity.mk)\n' > $@
	@echo '#include "Define.h"\n#include "Tools.h"\n\nNS_SA_EXT_BEGIN(tools)' >> $@
	@echo -n "const char * getCompileDate() { return __DATE__ \" \" __TIME__; }\n" >> $@
	@echo -n "Time getCompileUnixTime() { return SP_COMPILE_TIME; }\n" >> $@
	@for file in $^; do \
		echo "\n// $$file" "\nstatic VirtualFile" >> $@; \
		echo -n $$file | tr "/." _ >> $@; \
		echo -n " = VirtualFile::add(\"" >> $@; \
		echo -n $$file | sed "s:^$(GLOBAL_ROOT)/serenity/virtual::" >> $@; \
		echo -n "\", R\"VirtualFile(" >> $@; \
		cat $$file >> $@; \
		echo ")VirtualFile\");" >> $@; \
		echo "[VirtualFile] $$file > $@"; \
	done
	@echo 'NS_SA_EXT_END(tools)' >> $@

$(SERENITY_OUTPUT_DIR)/%.o: $(GLOBAL_ROOT)/%.cpp $(SERENITY_H_GCH) $(SERENITY_GCH)
	$(call sp_compile_cpp,$(SERENITY_CXXFLAGS))

$(SERENITY_OUTPUT_DIR)/%.o: $(GLOBAL_ROOT)/%.c $(SERENITY_H_GCH) $(SERENITY_GCH)
	$(call sp_compile_c,$(SERENITY_CFLAGS))

$(SERENITY_OUTPUT): $(SERENITY_H_GCH) $(SERENITY_GCH) $(SERENITY_OBJS)
	$(GLOBAL_QUIET_LINK) $(GLOBAL_CPP)  $(SERENITY_OBJS) $(SERENITY_LIBS) -shared -rdynamic -o $(SERENITY_OUTPUT)

libserenity: $(SERENITY_OUTPUT)
serenity: libserenity
	@echo "=== Install mod_serenity into Apache ($(STAPPLER_REPO_TAG)) ==="
	$(APXS) -i -n serenity -a $(SERENITY_OUTPUT)
	@echo "=== Complete! ==="

.PHONY: libserenity serenity serenity-version
