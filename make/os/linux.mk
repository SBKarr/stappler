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
OSTYPE_CPPFLAGS :=  -Wno-overloaded-virtual -frtti
OSTYPE_COMMON_LIBS := -lpthread -l:libcurl.a -l:libmbedtls.a -l:libmbedx509.a -l:libmbedcrypto.a -l:libpng.a -l:libjpeg.a -lz
OSTYPE_SERENITY_LIBS := -l:libcurl.a -l:libmbedtls.a -l:libmbedx509.a -l:libmbedcrypto.a -l:libpng.a -l:libjpeg.a -l:libfreetype.a
OSTYPE_CLI_LIBS += $(OSTYPE_COMMON_LIBS) -l:libsqlite3.a -ldl
OSTYPE_STAPPLER_LIBS += $(OSTYPE_CLI_LIBS) -l:libhyphen.a -l:libglfw3.a -l:libfreetype.a -lGLEW -lGL -lXxf86vm -lX11 -lXrandr -lXi -lXinerama -lXcursor
OSTYPE_LDFLAGS := -Wl,-z,defs -rdynamic -fuse-ld=gold
OSTYPE_EXEC_FLAGS := -fuse-ld=gold