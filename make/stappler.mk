STAPPLER_OUTPUT_DIR = $(TOOLKIT_OUTPUT)/stappler
STAPPLER_OUTPUT = $(TOOLKIT_OUTPUT)/libstappler.so
STAPPLER_OUTPUT_STATIC = $(TOOLKIT_OUTPUT)/libstappler.a

STAPPLER_PRECOMPILED_HEADERS += \
	stappler/src/core/SPDefine.h \
	common/core/SPCore.h \
	common/core/SPCommon.h

STAPPLER_SRCS_DIRS += \
	common \
	stappler/src/actions \
	stappler/src/components \
	stappler/src/core \
	stappler/src/features \
	stappler/src/nodes \
	stappler/src/platform/universal \
	$(COCOS2D_STAPPLER_SRCS_DIRS)

STAPPLER_SRCS_OBJS += \
	$(COCOS2D_STAPPLER_SRCS_OBJS) \
	libs/src/nanovg/src/nanovg.c

STAPPLER_INCLUDES_DIRS += \
	common \
	stappler/src \

STAPPLER_INCLUDES_OBJS += \
	$(COCOS2D_STAPPLER_INCLUDES_OBJS) \
	$(OSTYPE_INCLUDE) \
	$(OSTYPE_INCLUDE)/cairo \
	$(GLOBAL_ROOT)/libs/src/nanovg/src


ifeq ($(shell uname -o),Cygwin)
STAPPLER_SRCS_DIRS += $(COCOS2D_ROOT)/cocos/platform/win32 stappler/src/platform/win32
ifndef LOCAL_MAIN
STAPPLER_SRCS_DIRS += stappler/src/platform/win32_main
endif
else
STAPPLER_SRCS_DIRS += $(COCOS2D_ROOT)/cocos/platform/linux stappler/src/platform/linux
STAPPLER_INCLUDES_OBJS += $(OSTYPE_INCLUDE)/freetype2/linux
ifndef LOCAL_MAIN
STAPPLER_SRCS_DIRS += stappler/src/platform/linux_main
endif
endif


STAPPLER_LIBS := -L$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH) $(OSTYPE_STAPPLER_LIBS)

STAPPLER_SRCS := \
	$(foreach dir,$(STAPPLER_SRCS_DIRS),$(shell find $(GLOBAL_ROOT)/$(dir) -name '*.c*')) \
	$(addprefix $(GLOBAL_ROOT)/,$(STAPPLER_SRCS_OBJS))

STAPPLER_INCLUDES := \
	$(foreach dir,$(STAPPLER_INCLUDES_DIRS),$(shell find $(GLOBAL_ROOT)/$(dir) -type d)) \
	$(addprefix $(GLOBAL_ROOT)/,$(STAPPLER_INCLUDES_OBJS))

STAPPLER_GCH := $(addprefix $(GLOBAL_ROOT)/,$(STAPPLER_PRECOMPILED_HEADERS))
STAPPLER_H_GCH := $(patsubst $(GLOBAL_ROOT)/%,$(STAPPLER_OUTPUT_DIR)/include/%,$(STAPPLER_GCH))
STAPPLER_GCH := $(addsuffix .gch,$(STAPPLER_H_GCH))

STAPPLER_OBJS := $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(patsubst $(GLOBAL_ROOT)/%,$(STAPPLER_OUTPUT_DIR)/%,$(STAPPLER_SRCS))))
STAPPLER_DIRS := $(sort $(dir $(STAPPLER_OBJS))) $(sort $(dir $(STAPPLER_GCH)))

STAPPLER_INPUT_CFLAGS := $(addprefix -I,$(sort $(dir $(STAPPLER_GCH)))) $(addprefix -I,$(STAPPLER_INCLUDES))  -DCC_STATIC

STAPPLER_CXXFLAGS := $(GLOBAL_CXXFLAGS) $(STAPPLER_INPUT_CFLAGS)
STAPPLER_CFLAGS := $(GLOBAL_CFLAGS) $(STAPPLER_INPUT_CFLAGS)

-include $(patsubst %.o,%.d,$(STAPPLER_OBJS))
-include $(patsubst %.gch,%.d,$(STAPPLER_GCH))

$(STAPPLER_OUTPUT_DIR)/include/%.h : $(GLOBAL_ROOT)/%.h
	@$(GLOBAL_MKDIR) $(dir $(STAPPLER_OUTPUT_DIR)/include/$*.h)
	@cp -f $< $(STAPPLER_OUTPUT_DIR)/include/$*.h

$(STAPPLER_OUTPUT_DIR)/include/%.h.gch: $(STAPPLER_OUTPUT_DIR)/include/%.h
	$(GLOBAL_QUIET_CPP) $(GLOBAL_CPP) -MMD -MP -MF $(STAPPLER_OUTPUT_DIR)/include/$*.d $(STAPPLER_CXXFLAGS) -c -o $@ $<

$(STAPPLER_OUTPUT_DIR)/%.o: $(GLOBAL_ROOT)/%.cpp $(STAPPLER_H_GCH) $(STAPPLER_GCH)
	$(GLOBAL_QUIET_CPP) $(GLOBAL_CPP) -MMD -MP -MF $(STAPPLER_OUTPUT_DIR)/$*.d $(STAPPLER_CXXFLAGS) -c -o $@ $<

$(STAPPLER_OUTPUT_DIR)/%.o: $(GLOBAL_ROOT)/%.c $(STAPPLER_H_GCH) $(STAPPLER_GCH)
	$(GLOBAL_QUIET_CC) $(GLOBAL_CC) -MMD -MP -MF $(STAPPLER_OUTPUT_DIR)/$*.d $(STAPPLER_CFLAGS) -c -o $@ $<

$(STAPPLER_OUTPUT): $(STAPPLER_H_GCH) $(STAPPLER_GCH) $(STAPPLER_OBJS)
	$(GLOBAL_QUIET_LINK) $(GLOBAL_CPP) $(STAPPLER_OBJS) $(STAPPLER_LIBS) -shared $(OSTYPE_LDFLAGS) -o $(STAPPLER_OUTPUT)

$(STAPPLER_OUTPUT_STATIC) : $(STAPPLER_H_GCH) $(STAPPLER_GCH) $(STAPPLER_OBJS)
	$(GLOBAL_QUIET_LINK) $(GLOBAL_AR) $(STAPPLER_OUTPUT_STATIC) $(STAPPLER_OBJS)

libstappler: .prebuild_stappler $(STAPPLER_OUTPUT) $(STAPPLER_OUTPUT_STATIC)

.prebuild_stappler:
	@echo "=== Build libstappler ==="
	@$(GLOBAL_MKDIR) $(STAPPLER_DIRS)

.PHONY: .prebuild_stappler libstappler
