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

.DEFAULT_GOAL := local

IS_LOCAL_BUILD := 1

BUILD_OUTDIR := $(LOCAL_OUTDIR)/objs

GLOBAL_ROOT := $(STAPPLER_ROOT)
GLOBAL_OUTPUT := $(BUILD_OUTDIR)

include $(GLOBAL_ROOT)/make/utils/compiler.mk

BUILD_OUTDIR := $(BUILD_OUTDIR)/$(GLOBAL_CC)/$(if $(RELEASE),release,debug)/local

include $(GLOBAL_ROOT)/make/utils/external.mk

all: local

local: .prebuild_local .local_prebuild $(LOCAL_OUTPUT_LIBRARY) $(LOCAL_OUTPUT_EXECUTABLE)
.preclean: .local_preclean

$(LOCAL_OUTPUT_EXECUTABLE) : $(BUILD_OBJS) $(BUILD_MAIN_OBJ)
	$(GLOBAL_QUIET_LINK) $(GLOBAL_CPP) $(BUILD_OBJS) $(BUILD_MAIN_OBJ) $(BUILD_LIBS) $(TOOLKIT_LIB_FLAGS) $(OSTYPE_EXEC_FLAGS) -o $(LOCAL_OUTPUT_EXECUTABLE)

$(LOCAL_OUTPUT_LIBRARY): $(BUILD_OBJS)
	$(GLOBAL_QUIET_LINK) $(GLOBAL_CPP) -shared  $(BUILD_OBJS) $(BUILD_LIBS) $(TOOLKIT_LIB_FLAGS) $(OSTYPE_EXEC_FLAGS) -o $(LOCAL_OUTPUT_LIBRARY)

$(BUILD_OUTDIR)/%.o: /%.cpp $(TOOLKIT_H_GCH) $(TOOLKIT_GCH) 
	$(GLOBAL_QUIET_CPP) $(GLOBAL_CPP) -MMD -MP -MF $(BUILD_OUTDIR)/$*.d $(BUILD_CXXFLAGS) -c -o $@ `$(GLOBAL_ROOT)/convert-path.sh $<`

$(BUILD_OUTDIR)/%.o: /%.mm $(TOOLKIT_H_GCH) $(TOOLKIT_GCH) 
	$(GLOBAL_QUIET_CPP) $(GLOBAL_CPP) -MMD -MP -MF $(BUILD_OUTDIR)/$*.d $(BUILD_CXXFLAGS) -c -o $@ `$(GLOBAL_ROOT)/convert-path.sh $<`

$(BUILD_OUTDIR)/%.o: /%.c $(TOOLKIT_H_GCH) $(TOOLKIT_GCH)
	$(GLOBAL_QUIET_CC) $(GLOBAL_CC) -MMD -MP -MF $(BUILD_OUTDIR)/$*.d $(BUILD_CFLAGS) -c -o $@ `$(GLOBAL_ROOT)/convert-path.sh $<`

clean_local:
	$(GLOBAL_RM) $(LOCAL_OUTPUT_EXECUTABLE) $(LOCAL_OUTPUT_LIBRARY)

.PHONY: clean_local clean .prebuild_local .local_prebuild local .local_preclean all local

.preclean:
	$(GLOBAL_RM) $(LOCAL_OUTPUT_EXECUTABLE) $(LOCAL_OUTPUT_LIBRARY)

.prebuild_local:
	@echo "=== Begin build ==="
	@$(GLOBAL_MKDIR) $(BUILD_DIRS)
