# Copyright (c) 2016-2020 Roman Katuntsev <sbkarr@stappler.org>
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

MATERIAL_OUTPUT_DIR = $(abspath $(TOOLKIT_OUTPUT)/material)
MATERIAL_OUTPUT_STATIC = $(abspath $(TOOLKIT_OUTPUT)/libmaterial.a)

MATERIAL_PRECOMPILED_HEADERS += \
	components/material/Material.h

MATERIAL_SRCS_DIRS += \
	components/material

MATERIAL_SRCS_OBJS +=

MATERIAL_INCLUDES_DIRS += \
	components/material \
	components/common \
	components/layout \
	components/stappler/src

MATERIAL_INCLUDES_OBJS += \
	$(COCOS2D_STAPPLER_INCLUDES_OBJS) \
	$(OSTYPE_INCLUDE)


TOOLKIT_NAME := MATERIAL
TOOLKIT_TITLE := material

include $(GLOBAL_ROOT)/make/utils/toolkit.mk

$(MATERIAL_OUTPUT_STATIC) : $(MATERIAL_H_GCH) $(MATERIAL_GCH) $(MATERIAL_OBJS)
	$(GLOBAL_QUIET_LINK) $(GLOBAL_AR) $(MATERIAL_OUTPUT_STATIC) $(MATERIAL_OBJS)

libmaterial: libstappler $(MATERIAL_OUTPUT_STATIC)

.PHONY: libmaterial
