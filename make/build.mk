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

GLOBAL_RM ?= rm -f
GLOBAL_CP ?= cp -f
GLOBAL_MAKE ?= make
GLOBAL_MKDIR ?= mkdir -p
GLOBAL_AR ?= ar rcs

BUILD_CURRENT_COUNTER ?= 1
BUILD_FILES_COUNTER ?= 0

TOOLKIT_CLEARABLE_OUTPUT := $(TOOLKIT_OUTPUT)

ifeq ($(LOCAL_TOOLKIT),serenity)
ifeq ($(IS_LOCAL_BUILD),1)
TOOLKIT_CLEARABLE_OUTPUT :=
endif
endif

ifeq ($(UNAME),Cygwin)
GLOBAL_PREBUILT_LIBS_PATH := $(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)
else
GLOBAL_PREBUILT_LIBS_PATH := $(realpath $(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH))
endif

# cocos2d deps
-include $(GLOBAL_ROOT)/$(COCOS2D_ROOT)/cocos2d.mk

# libstappler + libcocos2d
include $(GLOBAL_ROOT)/make/stappler.mk

# libmaterial
include $(GLOBAL_ROOT)/make/material.mk

# libcli - stappler command line toolkit
include $(GLOBAL_ROOT)/make/cli.mk

# libspmin - stappler embeddable minimal toolkit with no cocos2d-x
include $(GLOBAL_ROOT)/make/spmin.mk

# serenity - apache httpd module. based on stappler toolkit
include $(GLOBAL_ROOT)/make/serenity.mk

ifdef verbose
ifneq ($(verbose),yes)
GLOBAL_QUIET_CC = @ echo [$(BUILD_LIBRARY): $$(($(BUILD_CURRENT_COUNTER)*100/$(BUILD_FILES_COUNTER)))% $(BUILD_CURRENT_COUNTER)/$(BUILD_FILES_COUNTER)] [$(notdir $(GLOBAL_CC))] $@ ;
GLOBAL_QUIET_CPP = @ echo [$(BUILD_LIBRARY): $$(($(BUILD_CURRENT_COUNTER)*100/$(BUILD_FILES_COUNTER)))% $(BUILD_CURRENT_COUNTER)/$(BUILD_FILES_COUNTER)] [$(notdir $(GLOBAL_CPP))] $@ ;
GLOBAL_QUIET_LINK = @ echo [Link] $@ ;
endif
else
GLOBAL_QUIET_CC = @ echo [$(BUILD_LIBRARY): $$(($(BUILD_CURRENT_COUNTER)*100/$(BUILD_FILES_COUNTER)))% $(BUILD_CURRENT_COUNTER)/$(BUILD_FILES_COUNTER)] [$(notdir $(GLOBAL_CC))] $@ ;
GLOBAL_QUIET_CPP = @ echo [$(BUILD_LIBRARY): $$(($(BUILD_CURRENT_COUNTER)*100/$(BUILD_FILES_COUNTER)))% $(BUILD_CURRENT_COUNTER)/$(BUILD_FILES_COUNTER)] [$(notdir $(GLOBAL_CPP))] $@ ;
GLOBAL_QUIET_LINK = @ echo [Link] $@ ;
endif

.PHONY: clean .postbuild .preclean

clean: .preclean
	$(GLOBAL_RM) -r $(TOOLKIT_CLEARABLE_OUTPUT) $(GLOBAL_OUTPUT)
