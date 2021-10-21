# Copyright (c) 2018 Roman Katuntsev <sbkarr@stappler.org>
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

ifndef STAPPLER_TARGET

.DEFAULT_GOAL := host

host: host-debug
clean: host-debug-clean
install: host-install

host-install:
	@$(MAKE) STAPPLER_TARGET=host install

host-debug:
	@$(MAKE) STAPPLER_TARGET=host

host-debug-clean:
	@$(MAKE) STAPPLER_TARGET=host clean

host-release:
	@$(MAKE) STAPPLER_TARGET=host RELEASE=1

host-release-clean:
	@$(MAKE) STAPPLER_TARGET=host RELEASE=1 clean

android: android-debug
android-clean: android-debug-clean

android-mk:
	@$(MAKE) ANDROID_EXPORT=1 STAPPLER_TARGET=android android-export

android-debug:
	@$(MAKE) STAPPLER_TARGET=android android

android-debug-clean:
	@$(MAKE) STAPPLER_TARGET=android android-clean

android-release:
	@$(MAKE) STAPPLER_TARGET=android RELEASE=1 android

android-release-clean:
	@$(MAKE) STAPPLER_TARGET=android RELEASE=1 android-clean

ios: ios-debug
ios-clean: ios-debug-clean

ios-export:
	@$(MAKE) STAPPLER_TARGET=ios IOS_ARCH=export ios-export

ios-debug:
	@$(MAKE) STAPPLER_TARGET=ios ios

ios-debug-clean:
	@$(MAKE) STAPPLER_TARGET=ios ios-clean

ios-release:
	@$(MAKE) STAPPLER_TARGET=ios RELEASE=1 ios

ios-release-clean:
	@$(MAKE) STAPPLER_TARGET=ios RELEASE=1 ios-clean

ios-all:
	@$(MAKE) STAPPLER_TARGET=ios ios
	@$(MAKE) STAPPLER_TARGET=ios RELEASE=1 ios

ios-all-clean:
	@$(MAKE) STAPPLER_TARGET=ios ios-clean
	@$(MAKE) STAPPLER_TARGET=ios RELEASE=1 ios-clean

.PHONY: clean install
.PHONY: host host-clean host-debug host-debug-clean host-release host-release-clean host-install
.PHONY: android android-clean android-mk android-debug android-debug-clean android-release android-release-clean
.PHONY: ios ios-clean ios-debug ios-debug-clean ios-release ios-release-clean

else

ifeq ($(STAPPLER_TARGET),host)
include $(STAPPLER_ROOT)/make/local.mk
else ifeq ($(STAPPLER_TARGET),android)
include $(STAPPLER_ROOT)/make/android.standalone.mk
else ifeq ($(STAPPLER_TARGET),ios)
include $(STAPPLER_ROOT)/make/ios.standalone.mk
endif

endif
