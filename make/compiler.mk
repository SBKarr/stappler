
ifeq ($(shell uname -o),Cygwin)
OSTYPE_PREBUILT_PATH := libs/win32/x86_64/lib
OSTYPE_INCLUDE := libs/win32/x86_64/include
OSTYPE_CFLAGS := -DCYGWIN -Wall -Wno-overloaded-virtual -frtti -D_WIN32 -DGLEW_STATIC
OSTYPE_COMMON_LIBS := -static -lpng -ljpeg -lcurl -lmbedx509 -lmbedtls -lmbedcrypto -lz -lws2_32
OSTYPE_CLI_LIBS += $(OSTYPE_COMMON_LIBS) -lsqlite3
OSTYPE_STAPPLER_LIBS += $(OSTYPE_CLI_LIBS) -lsqlite3 -lglfw3 -lglew32 -lopengl32 -lGdi32 -lcairo -lpixman-1 -lfreetype
OSTYPE_LDFLAGS := -municode
OSTYPE_EXEC_FLAGS := -static-libgcc -static-libstdc++
else
OSTYPE_PREBUILT_PATH := libs/linux/x86_64/lib
OSTYPE_INCLUDE :=  libs/linux/x86_64/include
OSTYPE_CFLAGS := -DLINUX -Wall -Wno-overloaded-virtual -fPIC -frtti
OSTYPE_COMMON_LIBS := -lpthread -l:libcurl.a -l:libmbedtls.a -l:libmbedx509.a -l:libmbedcrypto.a -l:libpng.a -l:libjpeg.a -lz
OSTYPE_CLI_LIBS += $(OSTYPE_COMMON_LIBS) -l:libsqlite3.a -ldl
OSTYPE_STAPPLER_LIBS += $(OSTYPE_CLI_LIBS) -l:libhyphen.a -l:libglfw3.a -l:libcairo.a -l:libpixman-1.a -l:libfreetype.a -lGLEW -lGL -lXxf86vm -lX11 -lXrandr -lXi -lXinerama -lXcursor
OSTYPE_LDFLAGS := -Wl,-z,defs -rdynamic
OSTYPE_EXEC_FLAGS := 
endif

ifndef GLOBAL_CPP
ifdef CLANG
ifeq ($(CLANG),1)
GLOBAL_CPP := clang++
else
GLOBAL_CPP := clang++-$(CLANG)
endif
else
ifdef MINGW
GLOBAL_CPP := $(MINGW)-g++
else
GLOBAL_CPP := g++
endif
endif
endif

ifndef GLOBAL_CC
ifdef CLANG
ifeq ($(CLANG),1)
GLOBAL_CC := clang
else
GLOBAL_CC := clang-$(CLANG)
endif
else
ifdef MINGW
GLOBAL_CC := $(MINGW)-gcc
else
GLOBAL_CC := gcc
endif
endif
endif
