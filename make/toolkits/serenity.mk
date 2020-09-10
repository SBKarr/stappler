# Copyright (c) 2016-2020 Roman Katuntsev <sbkarr@stappler.org>
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

APACHE_INCLUDE ?= /usr/local/include/apache

STAPPLER_REPO_TAG := $(shell $(abspath $(GLOBAL_ROOT))/make-version.sh $(abspath $(GLOBAL_ROOT)))

SERENITY_OUTPUT_DIR = $(abspath $(TOOLKIT_OUTPUT)/serenity)
SERENITY_OUTPUT = $(abspath $(TOOLKIT_OUTPUT)/mod_serenity.so)
SERENITY_SRCS_DIRS += \
	components/common \
	components/spug \
	components/serenity/apr \
	components/serenity/src \
	components/stellator/db \
	components/stellator/utils \
	components/layout/types

SERENITY_PRECOMPILED_HEADERS += \
	components/stellator/serenity_api/Define.h \
	components/common/core/SPCommon.h

SERENITY_FLAGS := -DSPAPR -DSERENITY

SERENITY_SRCS_OBJS += \
	components/document/src/mmd/common/MMDEngine.scu.cpp \
	components/document/src/mmd/internals/internal.scu.c \
	components/serenity/gen/__Version.cpp

SERENITY_SRCS_OBJS_ABS := \
	components/serenity/gen/__Virtual.cpp

SERENITY_INCLUDES_DIRS += \
	components/common \
	components/spug \
	components/serenity/src \
	components/serenity/apr \
	components/stellator/db \
	components/stellator/serenity_api \
	components/stellator/utils \

SERENITY_INCLUDES_OBJS += \
	$(OSTYPE_INCLUDE) \
	components/document/src/mmd/common \
	components/document/src/mmd/processors \
	components/serenity/ext/cityhash \
	components/layout/types \
	components/layout/style \
	components/layout/document \
	components/layout/vg \
	components/layout/vg/tess \
	components/layout \
	$(APACHE_INCLUDE)

SERENITY_VIRTUAL_SRCS := \
	$(shell find $(GLOBAL_ROOT)/components/serenity/virtual/js -name '*.js') \
	$(shell find $(GLOBAL_ROOT)/components/serenity/virtual/css -name '*.css') \
	$(shell find $(GLOBAL_ROOT)/components/serenity/virtual/html -name '*.html') \
	$(shell find $(GLOBAL_ROOT)/components/serenity/virtual/html -name '*.pug')

# Progress counter

TOOLKIT_NAME := SERENITY
TOOLKIT_TITLE := serenity

include $(GLOBAL_ROOT)/make/utils/toolkit.mk

$(SERENITY_OUTPUT_DIR)/include/%.h : /%.h
	@$(GLOBAL_MKDIR) $(dir $(SERENITY_OUTPUT_DIR)/include/$*.h)
	@cp -f $< $(SERENITY_OUTPUT_DIR)/include/$*.h

$(abspath $(GLOBAL_ROOT)/components/serenity/gen/__Virtual.cpp): $(SERENITY_VIRTUAL_SRCS)
	@mkdir -p $(GLOBAL_ROOT)/components/serenity/gen
	@printf '// Files for virtual filesystem\n// Autogenerated by makefile (serenity.mk)\n' > $@
	@printf '#include "Define.h"\n#include "Tools.h"\n\nNS_SA_EXT_BEGIN(tools)\n' >> $@
	@printf "\nconst char * getCompileDate() { return __DATE__ \" \" __TIME__; }\n" >> $@
	@printf "Time getCompileUnixTime() { return SP_COMPILE_TIME; }" >> $@
	@for file in $^; do \
		printf "\n\n// $$file \nstatic VirtualFile\n" >> $@; \
		printf $$file | tr "/." _ >> $@; \
		printf " = VirtualFile::add(\"" >> $@; \
		printf $$file | sed "s:^components/serenity/virtual::" >> $@; \
		printf "\", R\"VirtualFile(" >> $@; \
		cat $$file >> $@; \
		printf ")VirtualFile\");" >> $@; \
		echo "[VirtualFile] $$file > $@"; \
	done
	@echo 'NS_SA_EXT_END(tools)' >> $@

$(SERENITY_OUTPUT): $(SERENITY_H_GCH) $(SERENITY_GCH) $(SERENITY_OBJS)
	$(GLOBAL_QUIET_LINK) $(GLOBAL_CPP)  $(SERENITY_OBJS) $(SERENITY_LIBS) -shared -rdynamic -o $(SERENITY_OUTPUT)

libserenity: $(SERENITY_OUTPUT)
serenity: libserenity
	@echo "=== Install mod_serenity into Apache ($(STAPPLER_REPO_TAG)) ==="
	$(APXS) -i -n serenity -a $(SERENITY_OUTPUT)
	@echo "=== Complete! ==="


SERENITY_DEFAULT_CONFIG_START_SERVERS ?= 10
SERENITY_DEFAULT_CONFIG_DIRECTORY_INDEX ?= index.html

define SERENITY_DEFAULT_CONFIG
StartServers $(SERENITY_DEFAULT_CONFIG_START_SERVERS)
DirectoryIndex $(SERENITY_DEFAULT_CONFIG_DIRECTORY_INDEX)
LogLevel warn
LogFormat "%h %l %u %t \"%r\" %>s %b" common

SSLRandomSeed startup builtin
SSLRandomSeed connect builtin
SSLCipherSuite HIGH:MEDIUM:!aNULL:!MD5
SSLProtocol all -SSLv2 -SSLv3
SSLPassPhraseDialog  builtin
SSLSessionCache "shmcb:/home/apache/server/logs/ssl_scache(512000)"
SSLSessionCacheTimeout 300

LoadModule serenity_module $(abspath $(SERENITY_OUTPUT))

endef


SERENITY_DOCKER_CTRL_HTTPD ?= /serenity/apache/bin/httpd
SERENITY_DOCKER_CTRL_HTTPD_CONF ?= $(abspath $(GLOBAL_ROOT)/test/handlers/lib/httpd.conf)
SERENITY_DOCKER_CTRL_CONFIG ?= $(abspath $(GLOBAL_ROOT)/test/handlers/conf/serenity-docker.conf)

define SERENITY_DOCKER_CTRL
#!/bin/sh
CONFIG='$(abspath $(GLOBAL_ROOT)/components/serenity/config/bin/host/config)'
HTTPD='$(SERENITY_DOCKER_CTRL_HTTPD) -f $(SERENITY_DOCKER_CTRL_HTTPD_CONF)'
ACMD="$$1"

case $$ACMD in
stop|restart|graceful|graceful-stop)
    $$HTTPD -k "$$@"
    ERROR=$$?
    ;;
start)
	$$CONFIG $(SERENITY_DOCKER_CTRL_CONFIG) "$$@"
    $$HTTPD -k start
    ERROR=$$?
    ;;
start-sync)
	$$CONFIG $(SERENITY_DOCKER_CTRL_CONFIG) "$$@"
    $$HTTPD -k start -X
    ERROR=$$?
    ;;
*)
    $$HTTPD "$$@"
    ERROR=$$?
esac

exit $$ERROR
endef

.PHONY: libserenity serenity serenity-version
