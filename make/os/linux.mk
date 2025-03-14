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

OSTYPE_PREBUILT_PATH := libs/linux/x86_64/lib
OSTYPE_INCLUDE :=  libs/linux/x86_64/include
OSTYPE_CFLAGS := -DLINUX -Wall -fPIC
OSTYPE_CPPFLAGS :=  -Wno-overloaded-virtual -Wno-class-memaccess -frtti

OSTYPE_COMMON_LIBS := -lpthread \
	-l:libcurl.a \
	-l:libgnutls.a -l:libhogweed.a -l:libnettle.a -l:libgmp.a -l:libidn2.a -l:libunistring.a \
	-l:libngtcp2.a -l:libngtcp2_crypto_gnutls.a -l:libnghttp3.a \
	-l:libbrotlidec.a -l:libbrotlienc.a -l:libbrotlicommon.a \
	-l:libpng16.a -l:libgif.a -l:libjpeg.a -l:libwebp.a \
	-l:libbacktrace.a -lz

OSTYPE_SERENITY_LIBS := -lpthread \
	-l:libcurl.a -l:libfreetype.a -l:libngtcp2.a  -l:libnghttp3.a -l:libngtcp2_crypto_gnutls.a \
	-l:libgnutls.a -l:libhogweed.a -l:libnettle.a -l:libgmp.a -l:libidn2.a -l:libunistring.a \
	-l:libbrotlidec.a -l:libbrotlienc.a -l:libbrotlicommon.a \
	-l:libpng16.a -l:libgif.a -l:libjpeg.a -l:libwebp.a -l:libsqlite3.a \
	-l:libbacktrace.a -lz

OSTYPE_CLI_LIBS += $(OSTYPE_COMMON_LIBS) -l:libsqlite3.a -ldl

OSTYPE_STAPPLER_LIBS += $(OSTYPE_CLI_LIBS) -l:libglfw3.a -l:libfreetype.a \
	-lGLEW -lGL -lX11 -lXrandr -lXi -lXinerama -lXcursor

OSTYPE_STELLATOR_LIBS := $(OSTYPE_COMMON_LIBS) -l:libfreetype.a -l:libsqlite3.a -ldl

OSTYPE_LDFLAGS := -Wl,-z,defs -rdynamic -fuse-ld=gold
OSTYPE_EXEC_FLAGS := -fuse-ld=gold
