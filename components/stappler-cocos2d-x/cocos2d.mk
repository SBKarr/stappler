
COCOS2D_STAPPLER_SRCS_DIRS := \

COCOS2D_STAPPLER_SRCS_OBJS := \
	$(COCOS2D_ROOT)/CC2d.scu.cpp \
	$(COCOS2D_ROOT)/CCBase.scu.cpp \
	$(COCOS2D_ROOT)/CCRenderer.scu.cpp \
	$(COCOS2D_ROOT)/CCPlatform.scu.cpp \ \
	$(COCOS2D_ROOT)/external/lz4/lib/lz4.c

COCOS2D_STAPPLER_INCLUDES_OBJS := \
	$(COCOS2D_ROOT)/cocos \
	$(COCOS2D_ROOT)/cocos/platform \
	$(COCOS2D_ROOT)/external \
	$(COCOS2D_ROOT)/external/tinyxml2 \
	$(COCOS2D_ROOT)/external/ConvertUTF \
	$(COCOS2D_ROOT)/external/unzip \
	$(COCOS2D_ROOT)/external/lz4/lib \
	$(COCOS2D_ROOT)/external/xxhash

COCOS2D_CLI_SRCS_DIRS := \

COCOS2D_CLI_SRCS_OBJS := \
	$(COCOS2D_ROOT)/CCCli.scu.cpp \
	$(COCOS2D_ROOT)/cocos/platform/apple/CCFileUtils-apple.mm \
	$(COCOS2D_ROOT)/external/lz4/lib/lz4.c

COCOS2D_CLI_INCLUDES_OBJS := \
	$(COCOS2D_ROOT)/cocos \
	$(COCOS2D_ROOT)/cocos/platform/android \
	$(COCOS2D_ROOT)/cocos/platform/desktop \
	$(COCOS2D_ROOT)/cocos/platform/linux \
	$(COCOS2D_ROOT)/cocos/platform/windows \
	$(COCOS2D_ROOT)/external/tinyxml2 \
	$(COCOS2D_ROOT)/external/ConvertUTF \
	$(COCOS2D_ROOT)/external/unzip \
	$(COCOS2D_ROOT)/external/lz4/lib \
	$(COCOS2D_ROOT)/external/xxhash

ifdef ANDROID_ARCH

COCOS2D_STAPPLER_SRCS_DIRS +=
COCOS2D_STAPPLER_INCLUDES_OBJS += $(COCOS2D_ROOT)/cocos/platform/android

else ifdef IOS_ARCH

COCOS2D_STAPPLER_SRCS_OBJS += $(COCOS2D_ROOT)/CCPlatformApple.scu.mm
COCOS2D_STAPPLER_SRCS_DIRS += $(COCOS2D_ROOT)/cocos/platform/apple
COCOS2D_STAPPLER_INCLUDES_OBJS += $(COCOS2D_ROOT)/cocos/platform/ios $(COCOS2D_ROOT)/cocos/platform/apple

else

COCOS2D_STAPPLER_SRCS_DIRS += 
COCOS2D_STAPPLER_INCLUDES_OBJS += \
	$(COCOS2D_ROOT)/cocos/platform/desktop \
	$(COCOS2D_ROOT)/cocos/platform/linux \
	$(COCOS2D_ROOT)/cocos/platform/win32

endif
