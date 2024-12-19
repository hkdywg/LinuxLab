##
## build_core/build_app.mk
##
## History:
##    2024/04/12 - [yinwg] Created file
##
## 	This program is free software; you can redistribute it and/r modify
##  it under the terms of the GNU General Public License version 2 as
##  published by the Free Software Foundation.
##

$(if $(LOCAL_TARGET),,$(error $(LOCAL_PATH): LOCAL_TARGET is not defined))

# if any .cpp files existed, build all files as c++ codes.
LOCAL_CXX := $(if $(filter %.cpp,$(LOCAL_SRCS)),$(CXX),$(CC))

# convert .c file to objs
LOCAL_OBJS_C := $(filter %.o, $(patsubst $(PACKAGE_BUILD_TOPDIR)/%.c, $(PACKAGE_BUILD_OUTPATH)/%.o, $(LOCAL_SRCS)))

# convert .cpp file to objs
LOCAL_OBJS_CPP := $(filter %.o, $(patsubst $(PACKAGE_BUILD_TOPDIR)/%.cpp, $(PACKAGE_BUILD_OUTPATH)/%.o, $(LOCAL_SRCS)))

# convert .S file to objs
LOCAL_OBJS_S := $(filter %.o, $(patsubst $(PACKAGE_BUILD_TOPDIR)/%.S, $(PACKAGE_BUILD_OUTPATH)/%.o, $(LOCAL_SRCS)))

# convert .asm file to objs
LOCAL_OBJS_ASM := $(filter %.o, $(patsubst $(PACKAGE_BUILD_TOPDIR)/%.asm, $(PACKAGE_BUILD_OUTPATH)/%.o, $(LOCAL_SRCS)))

# final local objs
PRIVATE_OBJS := $(LOCAL_OBJS_C) $(LOCAL_OBJS_CPP) $(LOCAL_OBJS_S) $(LOCAL_OBJS_ASM) $(LOCAL_OBJS_DEPEND)

# define variables owned by specific $(LOCAL_LIBS)
PRIVATE_LIBS := $(patsubst lib%.so, -l%, $(filter %.so,$(LOCAL_LIBS)))
PRIVATE_LIBS += $(patsubst lib%.a, -l%, $(filter %.a,$(LOCAL_LIBS)))

# final targets
LOCAL_MODULE := $(patsubst $(PACKAGE_BUILD_TOPDIR)/%, $(PACKAGE_BUILD_OUTPATH)/%, $(LOCAL_PATH)/$(LOCAL_TARGET))

$(LOCAL_MODULE): PRIVATE_CXX := $(LOCAL_CXX)
$(LOCAL_MODULE): PRIVATE_AS := $(AS)
$(LOCAL_MODULE): PRIVATE_C := $(CC)
$(LOCAL_MODULE): PRIVATE_CFLAGS := $(LOCAL_CFLAGS)
$(LOCAL_MODULE): PRIVATE_AFLAGS := $(LOCAL_AFLAGS)
$(LOCAL_MODULE): PRIVATE_LDFLAGS := $(PRIVATE_LIBS) $(LOCAL_LDFLAGS)
$(LOCAL_MODULE): PRIVATE_SO_FLAGS := -Wl,-soname,$(LOCAL_SO_NAME)

# compile
$(PACKAGE_BUILD_OUTPATH)/%.o: $(PACKAGE_BUILD_TOPDIR)/%.c $(LOCAL_PATH)/make.inc
	@mkdir -p $(dir $@)
	$(MAKEFILE_V)$(PRIVATE_CXX) $(PRIVATE_CFLAGS) -MMD -c $< -o $@

$(PACKAGE_BUILD_OUTPATH)/%.o: $(PACKAGE_BUILD_TOPDIR)/%.cpp $(LOCAL_PATH)/make.inc
	@mkdir -p $(dir $@)
	$(MAKEFILE_V)$(PRIVATE_CXX) $(PRIVATE_CFLAGS) -MMD -c $< -o $@

$(PACKAGE_BUILD_OUTPATH)/%.o: $(PACKAGE_BUILD_TOPDIR)/%.S $(LOCAL_PATH)/make.inc
	@mkdir -p $(dir $@)
	$(if $(findstring COMPILE_WITH_AS, $(PRIVATE_CFLAGS)), \
	$(MAKEFILE_V)$(PRIVATE_AS) $(PRIVATE_AFLAGS) -o $@ $<, \
	$(MAKEFILE_V)$(PRIVATE_C) $(PRIVATE_CFLAGS) -c $< -o $@)

$(PACKAGE_BUILD_OUTPATH)/%.o: $(PACKAGE_BUILD_TOPDIR)/%.asm $(LOCAL_PATH)/make.inc
	@mkdir -p $(dir $@)
	$(MAKEFILE_V)$(PRIVATE_AS) $(PRIVATE_AFLAGS) -o $@ $<

# link
PRIVATE_MODULE_TYPE := $(if $(filter %.a, $(LOCAL_MODULE)), static_library, \
			$(if $(filter %.so, $(LOCAL_MODULE)), shared_library, executable))


$(LOCAL_MODULE): $(PRIVATE_OBJS) $(filter %.a, $(LOCAL_SRCS)) $(LOCAL_LIBS) $(LOCAL_PATH)/make.inc
	@mkdir -p $(dir $@)
	@mkdir -p $(USER_FILE_INSTALL_DIR)/test_dir
	$(if $(findstring static_library, $(PRIVATE_MODULE_TYPE)), $(build-static-library))
	$(if $(findstring shared_library, $(PRIVATE_MODULE_TYPE)), \
		$(MAKEFILE_V)$(PRIVATE_CXX) $(PRIVATE_CFLAGS) $(PRIVATE_SO_FLAGS) -fPIC -shared -o $@ $(filter-out %.a %.so %make.inc, $^))
	$(if $(findstring executable, $(PRIVATE_MODULE_TYPE)), \
		$(MAKEFILE_V)$(PRIVATE_CXX) $(PRIVATE_CFLAGS) $(PRIVATE_LDFLAGS) -o $@ $(filter-out %.so %make.inc, $^);\
		cp -rf $@ $(USER_FILE_INSTALL_DIR)/test_dir)

