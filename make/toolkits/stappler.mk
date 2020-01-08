# Copyright (c) 2018-2020 Roman Katuntsev <sbkarr@stappler.org>
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

STAPPLER_OUTPUT_DIR = $(abspath $(TOOLKIT_OUTPUT)/stappler)
STAPPLER_OUTPUT_STATIC = $(abspath $(TOOLKIT_OUTPUT)/libstappler.a)

STAPPLER_PRECOMPILED_HEADERS += \
	components/stappler/src/core/SPDefine.h \
	components/layout/SPLayout.h \
	components/common/core/SPCommon.h

STAPPLER_SRCS_DIRS += \
	components/common \
	components/layout \
	components/stappler/src/actions \
	components/stappler/src/components \
	components/stappler/src/core \
	components/stappler/src/features \
	components/stappler/src/nodes

STAPPLER_SRCS_OBJS += \
	components/stappler/src/platform/SPPlatform.scu.cpp \
	components/stappler/src/SPExtra.scu.cpp \
	$(COCOS2D_STAPPLER_SRCS_OBJS)

STAPPLER_INCLUDES_DIRS += \
	components/common \
	components/layout \
	components/stappler/src \

STAPPLER_INCLUDES_OBJS += \
	$(COCOS2D_STAPPLER_INCLUDES_OBJS) \
	$(OSTYPE_INCLUDE)

ifdef ANDROID_ARCH

STAPPLER_SRCS_DIRS += components/stappler/src/platform/android

else ifdef IOS_ARCH

STAPPLER_SRCS_DIRS += $(COCOS2D_STAPPLER_SRCS_DIRS)
STAPPLER_SRCS_OBJS += components/stappler/src/platform/SPPlatformApple.scu.mm
STAPPLER_INCLUDES_OBJS += $(COCOS2D_ROOT)/cocos/platform/ios

else

ifeq ($(UNAME),Darwin)
STAPPLER_SRCS_DIRS += $(COCOS2D_ROOT)/cocos/platform/mac $(COCOS2D_ROOT)/cocos/platform/apple
STAPPLER_SRCS_OBJS += components/components/stappler/src/platform/SPPlatformApple.scu.mm
ifndef LOCAL_MAIN
STAPPLER_SRCS_DIRS += components/stappler/src/platform/mac_main
endif
else ifeq ($(UNAME),Cygwin)
ifndef LOCAL_MAIN
STAPPLER_SRCS_DIRS += components/stappler/src/platform/win32_main
endif
else ifeq ($(UNAME),Msys)
ifndef LOCAL_MAIN
STAPPLER_SRCS_DIRS += components/stappler/src/platform/win32_main
endif
else
ifndef LOCAL_MAIN
STAPPLER_SRCS_DIRS += components/stappler/src/platform/linux_main
endif
endif

endif

STAPPLER_FLAGS := -DCC_STATIC

TOOLKIT_NAME := STAPPLER
TOOLKIT_TITLE := stappler

include $(GLOBAL_ROOT)/make/utils/toolkit.mk

$(STAPPLER_OUTPUT_DIR)/include/%.h : /%.h
	@$(GLOBAL_MKDIR) $(dir $(STAPPLER_OUTPUT_DIR)/include/$*.h)
	@cp -f $< $(STAPPLER_OUTPUT_DIR)/include/$*.h

$(STAPPLER_OUTPUT_STATIC) : $(STAPPLER_H_GCH) $(STAPPLER_GCH) $(STAPPLER_OBJS)
	$(GLOBAL_QUIET_LINK) $(GLOBAL_AR) $(STAPPLER_OUTPUT_STATIC) $(STAPPLER_OBJS)

libstappler: $(STAPPLER_OUTPUT_STATIC)

.PHONY: libstappler
