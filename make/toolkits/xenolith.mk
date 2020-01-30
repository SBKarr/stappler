# Copyright (c) 2020 Roman Katuntsev <sbkarr@stappler.org>
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

XENOLITH_OUTPUT_DIR = $(abspath $(TOOLKIT_OUTPUT)/xenolith)
XENOLITH_OUTPUT_STATIC = $(abspath $(TOOLKIT_OUTPUT)/libxenolith.a)

XENOLITH_PRECOMPILED_HEADERS += \
	components/xenolith/core/XLDefine.h \
	components/layout/SPLayout.h \
	components/common/core/SPCommon.h

XENOLITH_SRCS_DIRS += \
	components/common \
	components/layout \
	components/xenolith/core \

XENOLITH_SRCS_OBJS += \
	components/xenolith/platform/XLPlatform.scu.cpp

XENOLITH_INCLUDES_DIRS += \
	components/common \
	components/layout \
	components/xenolith \

XENOLITH_INCLUDES_OBJS += \
	$(OSTYPE_INCLUDE)

ifeq ($(UNAME),Darwin)

ifndef LOCAL_MAIN
XENOLITH_SRCS_DIRS += components/xenolith/platform/mac_main
endif

else ifeq ($(UNAME),Cygwin)

ifndef LOCAL_MAIN
XENOLITH_SRCS_DIRS += components/xenolith/platform/win32_main
endif

XENOLITH_FLAGS := -I$(shell cygpath -u $(VULKAN_SDK))/Include

else ifeq ($(UNAME),Msys)

ifndef LOCAL_MAIN
XENOLITH_SRCS_DIRS += components/xenolith/platform/win32_main
endif

XENOLITH_FLAGS := -I$(shell cygpath -u $(VULKAN_SDK))/Include

else

ifndef LOCAL_MAIN
XENOLITH_SRCS_DIRS += components/xenolith/platform/linux_main
endif

endif


TOOLKIT_NAME := XENOLITH
TOOLKIT_TITLE := xenolith

include $(GLOBAL_ROOT)/make/utils/toolkit.mk

$(XENOLITH_OUTPUT_DIR)/include/%.h : /%.h
	@$(GLOBAL_MKDIR) $(dir $(XENOLITH_OUTPUT_DIR)/include/$*.h)
	@cp -f $< $(XENOLITH_OUTPUT_DIR)/include/$*.h

$(XENOLITH_OUTPUT_STATIC) : $(XENOLITH_H_GCH) $(XENOLITH_GCH) $(XENOLITH_OBJS)
	$(GLOBAL_QUIET_LINK) $(GLOBAL_AR) $(XENOLITH_OUTPUT_STATIC) $(XENOLITH_OBJS)

libxenolith: $(XENOLITH_OUTPUT_STATIC)

.PHONY: libxenolith
