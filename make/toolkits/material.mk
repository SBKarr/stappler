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

MATERIAL_OUTPUT_DIR = $(TOOLKIT_OUTPUT)/material
MATERIAL_OUTPUT = $(TOOLKIT_OUTPUT)/libmaterial.so
MATERIAL_OUTPUT_STATIC = $(TOOLKIT_OUTPUT)/libmaterial.a

MATERIAL_PRECOMPILED_HEADERS += \
	material/Material.h

MATERIAL_SRCS_DIRS += \
	material

MATERIAL_SRCS_OBJS +=

MATERIAL_INCLUDES_DIRS += \
	material \
	common \
	layout \
	stappler/src

MATERIAL_INCLUDES_OBJS += \
	$(COCOS2D_STAPPLER_INCLUDES_OBJS) \
	$(OSTYPE_INCLUDE)


TOOLKIT_NAME := MATERIAL
TOOLKIT_TITLE := material

include $(GLOBAL_ROOT)/make/utils/toolkit.mk

$(MATERIAL_OUTPUT_DIR)/include/%.h : $(GLOBAL_ROOT)/%.h
	@$(GLOBAL_MKDIR) $(dir $(MATERIAL_OUTPUT_DIR)/include/$*.h)
	@cp -f $< $(MATERIAL_OUTPUT_DIR)/include/$*.h

$(MATERIAL_OUTPUT_DIR)/include/%.h.gch: $(MATERIAL_OUTPUT_DIR)/include/%.h
	$(GLOBAL_QUIET_CPP) $(GLOBAL_CPP) $(OSTYPE_GCHFLAGS) -MMD -MP -MF $(MATERIAL_OUTPUT_DIR)/include/$*.d $(MATERIAL_CXXFLAGS) -c -o $@ $<

$(MATERIAL_OUTPUT_DIR)/%.o: $(GLOBAL_ROOT)/%.cpp $(MATERIAL_H_GCH) $(MATERIAL_GCH)
	$(GLOBAL_QUIET_CPP) $(GLOBAL_CPP) -MMD -MP -MF $(MATERIAL_OUTPUT_DIR)/$*.d $(MATERIAL_CXXFLAGS) -c -o $@ $<

$(MATERIAL_OUTPUT_DIR)/%.o: $(GLOBAL_ROOT)/%.c $(MATERIAL_H_GCH) $(MATERIAL_GCH)
	$(GLOBAL_QUIET_CC) $(GLOBAL_CC) -MMD -MP -MF $(MATERIAL_OUTPUT_DIR)/$*.d $(MATERIAL_CFLAGS) -c -o $@ $<

$(MATERIAL_OUTPUT): $(MATERIAL_H_GCH) $(MATERIAL_GCH) $(STAPPLER_OBJS) $(MATERIAL_OBJS)
	$(GLOBAL_QUIET_LINK) $(GLOBAL_CPP) $(STAPPLER_OBJS) $(MATERIAL_OBJS) $(MATERIAL_LIBS) -shared $(OSTYPE_LDFLAGS) -o $(MATERIAL_OUTPUT)

$(MATERIAL_OUTPUT_STATIC) : $(MATERIAL_H_GCH) $(MATERIAL_GCH) $(MATERIAL_OBJS)
	$(GLOBAL_QUIET_LINK) $(GLOBAL_AR) $(MATERIAL_OUTPUT_STATIC) $(MATERIAL_OBJS)

libmaterial: .prebuild_material .prebuild_stappler $(MATERIAL_OUTPUT_STATIC)

.prebuild_material:
	@echo "=== Build libmaterial ==="
	@$(GLOBAL_MKDIR) $(MATERIAL_DIRS)

.PHONY: .prebuild_material libmaterial
