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

DOCUMENT_MAKEFILE_DIR := $(patsubst $(LOCAL_ROOT)/%,%,$(dir $(lastword $(MAKEFILE_LIST))))


DOCUMENT_SOURCE_DIR_COMMON := \
	$(DOCUMENT_MAKEFILE_DIR)src/mmd/common \
	$(DOCUMENT_MAKEFILE_DIR)src/mmd/internals

DOCUMENT_INCLUDE_COMMON := \
	$(DOCUMENT_MAKEFILE_DIR)src/mmd/common \
	$(DOCUMENT_MAKEFILE_DIR)src/mmd/processors


DOCUMENT_SOURCE_DIR_STAPPLER := \
	$(DOCUMENT_SOURCE_DIR_COMMON) \
	$(DOCUMENT_MAKEFILE_DIR)src/epub \
	$(DOCUMENT_MAKEFILE_DIR)src/mmd/layout

DOCUMENT_INCLUDE_STAPPLER := \
	$(DOCUMENT_INCLUDE_COMMON) \
	$(DOCUMENT_MAKEFILE_DIR)src/epub \
	$(DOCUMENT_MAKEFILE_DIR)src/mmd/layout


DOCUMENT_SOURCE_DIR_MATERIAL := \
	$(DOCUMENT_SOURCE_DIR_STAPPLER) \
	$(DOCUMENT_MAKEFILE_DIR)src/rich_text

DOCUMENT_INCLUDE_MATERIAL := \
	$(DOCUMENT_INCLUDE_STAPPLER) \
	$(DOCUMENT_MAKEFILE_DIR)src/rich_text \
	$(DOCUMENT_MAKEFILE_DIR)src/rich_text/articles \
	$(DOCUMENT_MAKEFILE_DIR)src/rich_text/common \
	$(DOCUMENT_MAKEFILE_DIR)src/rich_text/epub \
	$(DOCUMENT_MAKEFILE_DIR)src/rich_text/gallery \
	$(DOCUMENT_MAKEFILE_DIR)src/rich_text/menu

