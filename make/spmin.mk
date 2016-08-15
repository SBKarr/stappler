SPMIN_OUTPUT_DIR = $(TOOLKIT_OUTPUT)/common
SPMIN_OUTPUT = $(TOOLKIT_OUTPUT)/libcommon.so
SPMIN_OUTPUT_STATIC = $(TOOLKIT_OUTPUT)/libcommon.a

SPMIN_FLAGS := -DNOCC

SPMIN_PRECOMPILED_HEADERS += \
	common/core/SPCore.h \
	common/core/SPCommon.h

SPMIN_SRCS_DIRS += common
SPMIN_SRCS_OBJS += 
SPMIN_INCLUDES_DIRS += common
SPMIN_INCLUDES_OBJS += $(OSTYPE_INCLUDE)

SPMIN_LIBS += -L$(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH) $(OSTYPE_COMMON_LIBS)

SPMIN_SRCS := \
	$(foreach dir,$(SPMIN_SRCS_DIRS),$(shell find $(GLOBAL_ROOT)/$(dir) -name '*.c*')) \
	$(addprefix $(GLOBAL_ROOT)/,$(SPMIN_SRCS_OBJS))

SPMIN_INCLUDES := \
	$(foreach dir,$(SPMIN_INCLUDES_DIRS),$(shell find $(GLOBAL_ROOT)/$(dir) -type d)) \
	$(addprefix $(GLOBAL_ROOT)/,$(SPMIN_INCLUDES_OBJS))

SPMIN_GCH := $(addsuffix .gch,$(addprefix $(GLOBAL_ROOT)/,$(SPMIN_PRECOMPILED_HEADERS)))
SPMIN_GCH := $(patsubst $(GLOBAL_ROOT)/%,$(SPMIN_OUTPUT_DIR)/include/%,$(SPMIN_GCH))

SPMIN_OBJS := $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(patsubst $(GLOBAL_ROOT)/%,$(SPMIN_OUTPUT_DIR)/%,$(SPMIN_SRCS))))
SPMIN_DIRS := $(sort $(dir $(SPMIN_OBJS))) $(sort $(dir $(SPMIN_GCH)))

SPMIN_INPUT_CFLAGS := $(addprefix -I,$(sort $(dir $(SPMIN_GCH)))) $(addprefix -I,$(SPMIN_INCLUDES))

SPMIN_CXXFLAGS := $(GLOBAL_CXXFLAGS) $(SPMIN_FLAGS) $(SPMIN_INPUT_CFLAGS)
SPMIN_CFLAGS := $(GLOBAL_CFLAGS) $(SPMIN_FLAGS) $(SPMIN_INPUT_CFLAGS)

-include $(patsubst %.o,%.d,$(SPMIN_OBJS))
-include $(patsubst %.gch,%.d,$(SPMIN_GCH))

$(SPMIN_OUTPUT_DIR)/include/%.gch: $(GLOBAL_ROOT)/%
	@$(GLOBAL_MKDIR) $(dir $(SPMIN_OUTPUT_DIR)/include/$*)
	$(GLOBAL_QUIET_CPP) $(GLOBAL_CPP) -MMD -MP -MF $(SPMIN_OUTPUT_DIR)/include/$*.d $(SPMIN_CXXFLAGS) -c -o $@ $<
	@cp -f $< $(SPMIN_OUTPUT_DIR)/include/$*

$(SPMIN_OUTPUT_DIR)/%.o: $(GLOBAL_ROOT)/%.cpp $(SPMIN_GCH)
	$(GLOBAL_QUIET_CPP) $(GLOBAL_CPP) -MMD -MP -MF $(SPMIN_OUTPUT_DIR)/$*.d $(SPMIN_CXXFLAGS) -c -o $@ $<

$(SPMIN_OUTPUT_DIR)/%.o: $(GLOBAL_ROOT)/%.c $(SPMIN_GCH)
	$(GLOBAL_QUIET_CC) $(GLOBAL_CC) -MMD -MP -MF $(SPMIN_OUTPUT_DIR)/$*.d $(SPMIN_CFLAGS) -c -o $@ $<

$(SPMIN_OUTPUT): $(SPMIN_GCH) $(SPMIN_OBJS)
	$(GLOBAL_QUIET_LINK) $(GLOBAL_CPP)  $(SPMIN_OBJS) $(SPMIN_LIBS) -shared $(OSTYPE_LDFLAGS) -o $(SPMIN_OUTPUT)

$(SPMIN_OUTPUT_STATIC) : $(SPMIN_H_GCH) $(SPMIN_GCH) $(SPMIN_OBJS)
	$(GLOBAL_QUIET_LINK) $(GLOBAL_AR) $(SPMIN_OUTPUT_STATIC) $(SPMIN_OBJS)

libspmin: .prebuild_spmin $(SPMIN_OUTPUT) $(SPMIN_OUTPUT_STATIC)

.prebuild_spmin:
	@echo "=== Build libspmin ==="
	@$(GLOBAL_MKDIR) $(SPMIN_DIRS)

.PHONY: .prebuild_spmin libspmin
