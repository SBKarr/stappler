# Copyright (c) 2018-2019 Roman Katuntsev <sbkarr@stappler.org>
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

CLI_OUTPUT_DIR = $(abspath $(TOOLKIT_OUTPUT)/cli)
CLI_OUTPUT_STATIC = $(abspath $(TOOLKIT_OUTPUT)/libcli.a)

CLI_FLAGS := -DSP_RESTRICT -DSP_DRAW=0 -DSP_NO_STOREKIT -DCC_STATIC

CLI_PRECOMPILED_HEADERS += \
	components/stappler/src/core/SPDefine.h \
	components/layout/SPLayout.h \
	components/common/core/SPCommon.h

CLI_SRCS_DIRS += \
	components/common \
	components/spug \
	components/layout/document \
	components/layout/types \
	components/layout/vg \
	components/stappler/src/core \
	$(COCOS2D_CLI_SRCS_DIRS)

CLI_SRCS_OBJS += \
	components/stappler/src/platform/SPPlatform.scu.cpp \
	components/stappler/src/platform/SPPlatformApple.scu.mm \
	components/stappler/src/features/SPBasicFeatures.scu.cpp \
	$(COCOS2D_CLI_SRCS_OBJS)

CLI_INCLUDES_DIRS += \
	components/common \
	components/spug \
	components/layout \
	components/stappler/src

CLI_INCLUDES_OBJS += \
	$(COCOS2D_CLI_INCLUDES_OBJS) \
	$(OSTYPE_INCLUDE)

TOOLKIT_NAME := CLI
TOOLKIT_TITLE := cli

include $(GLOBAL_ROOT)/make/utils/toolkit.mk

$(CLI_OUTPUT_DIR)/include/%.h : /%.h
	@$(GLOBAL_MKDIR) $(dir $(CLI_OUTPUT_DIR)/include/$*.h)
	@cp -f $< $(CLI_OUTPUT_DIR)/include/$*.h

$(CLI_OUTPUT_DIR)/include/%.h.gch: $(CLI_OUTPUT_DIR)/include/%.h
	$(call sp_compile_gch,$(CLI_CXXFLAGS))

$(CLI_OUTPUT_DIR)/%.o: /%.cpp $(CLI_H_GCH) $(CLI_GCH)
	$(call sp_compile_cpp,$(CLI_CXXFLAGS))

$(CLI_OUTPUT_DIR)/%.o: /%.mm $(CLI_H_GCH) $(CLI_GCH)
	$(call sp_compile_mm,$(CLI_CXXFLAGS))

$(CLI_OUTPUT_DIR)/%.o: /%.c $(CLI_H_GCH) $(CLI_GCH)
	$(call sp_compile_c,$(CLI_CFLAGS))

$(CLI_OUTPUT_STATIC) : $(CLI_H_GCH) $(CLI_GCH) $(CLI_OBJS)
	$(GLOBAL_QUIET_LINK) $(GLOBAL_AR) $(CLI_OUTPUT_STATIC) $(CLI_OBJS)

libcli: $(CLI_OUTPUT_STATIC)

.PHONY: libcli
