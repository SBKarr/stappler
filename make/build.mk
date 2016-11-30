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

ifndef GLOBAL_ROOT
GLOBAL_ROOT = .
endif

ifndef COCOS2D_ROOT
COCOS2D_ROOT := libs/src/stappler-cocos2d-x
endif

include $(GLOBAL_ROOT)/make/compiler.mk

GLOBAL_RM ?= rm -f
GLOBAL_CP ?= cp -f
GLOBAL_MAKE ?= make
GLOBAL_MKDIR ?= mkdir -p
GLOBAL_AR ?= ar rcs

ifndef TOOLKIT_OUTPUT
TOOLKIT_OUTPUT := $(GLOBAL_ROOT)/build
endif

ifdef RELEASE
TOOLKIT_OUTPUT := $(TOOLKIT_OUTPUT)/$(notdir $(GLOBAL_CC))/release
ifeq ($(LOCAL_TOOLKIT),serenity)
GLOBAL_CFLAGS := -O2 -g -DNDEBUG $(OSTYPE_CFLAGS) $(GLOBAL_CFLAGS)
else
GLOBAL_CFLAGS := -Os -DNDEBUG $(OSTYPE_CFLAGS) $(GLOBAL_CFLAGS)
endif
else
TOOLKIT_OUTPUT := $(TOOLKIT_OUTPUT)/$(notdir $(GLOBAL_CC))/debug
GLOBAL_CFLAGS := -g -DDEBUG -DCOCOS2D_DEBUG=1 $(OSTYPE_CFLAGS) $(GLOBAL_CFLAGS)
endif

TOOLKIT_CLEARABLE_OUTPUT := $(TOOLKIT_OUTPUT)

ifeq ($(LOCAL_TOOLKIT),serenity)
ifeq ($(IS_LOCAL_BUILD),1)
TOOLKIT_CLEARABLE_OUTPUT :=
endif
endif

GLOBAL_CXXFLAGS := $(GLOBAL_CFLAGS) -std=gnu++14

ifeq ($(shell uname -o),Cygwin)
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
verbose:= yes
ifdef verbose
ifneq ($(verbose),yes)
GLOBAL_QUIET_CC = @ echo [Compile] $@ ;
GLOBAL_QUIET_CPP = @ echo [Compile++] $@ ;
GLOBAL_QUIET_LINK = @ echo [Link] $@ ;
endif
else
GLOBAL_QUIET_CC = @ echo [Compile] $@ ;
GLOBAL_QUIET_CPP = @ echo [Compile++] $@ ;
GLOBAL_QUIET_LINK = @ echo [Link] $@ ;
endif

.PHONY: clean .postbuild .preclean

clean: .preclean
	$(GLOBAL_RM) -r $(TOOLKIT_CLEARABLE_OUTPUT) $(GLOBAL_OUTPUT)
