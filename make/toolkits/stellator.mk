# Copyright (c) 2019 Roman Katuntsev <sbkarr@stappler.org>
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

STELLATOR_OUTPUT_DIR = $(abspath $(TOOLKIT_OUTPUT)/stellator)
STELLATOR_OUTPUT_STATIC = $(abspath $(TOOLKIT_OUTPUT)/libstellator.a)

STELLATOR_FLAGS := -DNOCC -DSTELLATOR

STELLATOR_PRECOMPILED_HEADERS += \
	components/common/core/SPCommon.h \
	components/stellator/core/STDefine.h

STELLATOR_SRCS_DIRS += \
	components/common \
	components/stellator \
	components/spug \
	components/layout/types

STELLATOR_SRCS_OBJS += 
STELLATOR_INCLUDES_DIRS += \
	components/common \
	components/stellator \
	components/spug

STELLATOR_INCLUDES_OBJS += \
	$(OSTYPE_INCLUDE) \
	components/layout/types \
	components/layout/style \
	components/layout/document \
	components/layout/vg \
	components/layout

TOOLKIT_NAME := STELLATOR
TOOLKIT_TITLE := stellator

include $(GLOBAL_ROOT)/make/utils/toolkit.mk

$(STELLATOR_OUTPUT_DIR)/include/%.h : /%.h
	@$(GLOBAL_MKDIR) $(dir $(STELLATOR_OUTPUT_DIR)/include/$*.h)
	@cp -f $< $(STELLATOR_OUTPUT_DIR)/include/$*.h

$(STELLATOR_OUTPUT_DIR)/include/%.h.gch: $(STELLATOR_OUTPUT_DIR)/include/%.h
	$(call sp_compile_gch,$(STELLATOR_CXXFLAGS))

$(STELLATOR_OUTPUT_DIR)/%.o: /%.cpp $(STELLATOR_H_GCH) $(STELLATOR_GCH)
	$(call sp_compile_cpp,$(STELLATOR_CXXFLAGS))

$(STELLATOR_OUTPUT_DIR)/%.o: /%.mm $(STELLATOR_H_GCH) $(STELLATOR_GCH)
	$(call sp_compile_mm,$(STELLATOR_CXXFLAGS))

$(STELLATOR_OUTPUT_DIR)/%.o: /%.c $(STELLATOR_H_GCH) $(STELLATOR_GCH)
	$(call sp_compile_c,$(STELLATOR_CFLAGS))

$(STELLATOR_OUTPUT_STATIC) : $(STELLATOR_H_GCH) $(STELLATOR_GCH) $(STELLATOR_OBJS)
	$(GLOBAL_QUIET_LINK) $(GLOBAL_AR) $(STELLATOR_OUTPUT_STATIC) $(STELLATOR_OBJS)

libstellator: $(STELLATOR_OUTPUT_STATIC)

.PHONY: libstellator
