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

ifeq ($(UNAME),Cygwin)

OSTYPE_PREBUILT_PATH := libs/win32/x86_64/lib
OSTYPE_INCLUDE := libs/win32/x86_64/include
OSTYPE_CFLAGS := -DCYGWIN -Wall -Wno-overloaded-virtual -frtti -D_WIN32 -DGLEW_STATIC
OSTYPE_COMMON_LIBS := -static -lpng -ljpeg -lcurl -lmbedx509 -lmbedtls -lmbedcrypto -lz -lws2_32
OSTYPE_CLI_LIBS += $(OSTYPE_COMMON_LIBS) -lsqlite3
OSTYPE_STAPPLER_LIBS += $(OSTYPE_CLI_LIBS) -lsqlite3 -lglfw3 -lglew32 -lopengl32 -lGdi32 -lcairo -lpixman-1 -lfreetype
OSTYPE_LDFLAGS := -municode
OSTYPE_EXEC_FLAGS := -static-libgcc -static-libstdc++

endif
