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

ifndef TOOLKIT_OUTPUT
TOOLKIT_OUTPUT := $(GLOBAL_ROOT)/build
endif

UNAME := $(shell uname)
ifneq ($(UNAME),Darwin)
UNAME := $(shell uname -o)
endif

include $(GLOBAL_ROOT)/make/os/android_common.mk
include $(GLOBAL_ROOT)/make/os/ios_common.mk

ifdef ANDROID_ARCH

include $(GLOBAL_ROOT)/make/os/android.mk

else ifdef IOS_ARCH

include $(GLOBAL_ROOT)/make/os/ios.mk

else # Begin desktop arch

ifeq ($(UNAME),Darwin)
include $(GLOBAL_ROOT)/make/os/darwin.mk
else ifeq ($(UNAME),Cygwin)
include $(GLOBAL_ROOT)/make/os/cygwin.mk
else
include $(GLOBAL_ROOT)/make/os/linux.mk
endif

ifndef GLOBAL_CPP

ifdef CLANG
ifeq ($(CLANG),1)
GLOBAL_CPP := clang++
else
GLOBAL_CPP := clang++-$(CLANG)
endif # ifeq ($(CLANG),1)
OSTYPE_GCHFLAGS := -x c++-header
else
ifdef MINGW
GLOBAL_CPP := $(MINGW)-g++
else
GLOBAL_CPP := g++
endif # ifdef MINGW
endif # ifdef CLANG

endif # ifndef GLOBAL_CPP

ifndef GLOBAL_CC
ifdef CLANG
ifeq ($(CLANG),1)
GLOBAL_CC := clang
else
GLOBAL_CC := clang-$(CLANG)
endif # ifeq ($(CLANG),1)
OSTYPE_GCHFLAGS := -x c++-header
else
ifdef MINGW
GLOBAL_CC := $(MINGW)-gcc
else
GLOBAL_CC := gcc
endif # ifdef MINGW
endif # ifdef CLANG
endif # ifndef GLOBAL_CC

ifdef RELEASE
TOOLKIT_OUTPUT := $(TOOLKIT_OUTPUT)/$(notdir $(GLOBAL_CC))/release
GLOBAL_CFLAGS := -O2 -g -DNDEBUG $(OSTYPE_CFLAGS) $(GLOBAL_CFLAGS)
else
TOOLKIT_OUTPUT := $(TOOLKIT_OUTPUT)/$(notdir $(GLOBAL_CC))/debug
GLOBAL_CFLAGS := -g -DDEBUG -DCOCOS2D_DEBUG=1 $(OSTYPE_CFLAGS) $(GLOBAL_CFLAGS)
endif # ifdef RELEASE

endif # End desktop arch

GLOBAL_CXXFLAGS := $(GLOBAL_CFLAGS) -DSTAPPLER -std=gnu++14 $(OSTYPE_CPPFLAGS)
GLOBAL_CFLAGS := $(GLOBAL_CFLAGS) -DSTAPPLER -std=c11
