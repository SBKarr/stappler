# Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>
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

STAPPLER_ROOT = ../../..

LOCAL_OUTDIR := stappler-build
LOCAL_EXECUTABLE := testapp

LOCAL_TOOLKIT := material

LOCAL_ROOT = .

include $(LOCAL_ROOT)/../make.mk

LOCAL_SRCS_DIRS :=  Classes $(DOCUMENT_SOURCE_DIR_MATERIAL)
LOCAL_SRCS_OBJS :=

LOCAL_INCLUDES_DIRS := Classes
LOCAL_INCLUDES_OBJS := $(DOCUMENT_INCLUDE_MATERIAL)

$(LOCAL_OUTDIR)/host/common:
	mkdir -p $(LOCAL_OUTDIR)/host
	cp -f -r $(LOCAL_ROOT)/common $(LOCAL_OUTDIR)/host/common

include $(STAPPLER_ROOT)/make/universal.mk

prepare-linux: $(LOCAL_OUTDIR)/host/common

host-install: prepare-linux
host-debug: prepare-linux
host-release: prepare-linux

.PHONY: prepare-linux
