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

ifeq ($(UNAME),Darwin)

OSTYPE_PREBUILT_PATH := libs/mac/x86_64/lib
OSTYPE_INCLUDE :=  libs/mac/x86_64/include
OSTYPE_CFLAGS := -DMACOSX  -DUSE_FILE32API -Wall -fPIC
OSTYPE_CPPFLAGS :=  -Wno-overloaded-virtual -frtti
OSTYPE_COMMON_LIBS := -lpthread \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libcurl.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libbrotlicommon.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libbrotlienc.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libbrotlidec.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libcurl.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libwebp.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libpng.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libjpeg.a \
	-framework Foundation -lz -framework Security

OSTYPE_CLI_LIBS += $(OSTYPE_COMMON_LIBS) \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libsqlite3.a \
	-ldl -framework SystemConfiguration

OSTYPE_STAPPLER_LIBS += $(OSTYPE_CLI_LIBS) \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libhyphen.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libglfw3.a \
	$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)/libfreetype.a \
	-framework OpenGL -framework CoreGraphics -framework Cocoa -framework IOKit -framework CoreVideo

OSTYPE_LDFLAGS :=
OSTYPE_EXEC_FLAGS :=
CLANG := 1
BUILD_OBJC := 1

endif
