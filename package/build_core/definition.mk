##
## build_core/definition.mk
##
## History:
##    2024/04/12 - [yinwg] Created file
##
## 	This program is free software; you can redistribute it and/r modify
##  it under the terms of the GNU General Public License version 2 as
##  published by the Free Software Foundation.
##

# Strip quotes and then whitespaces
qstrip = $(strip $(subst ",,$(1)))
#"))

###################################################
# Figure out where we are
###################################################

define my-dir
$(strip \
	$(eval md_file_ := $$(lastword $$(MAKEFILE_LIST))) \
	$(patsubst %/,%,$(dir $(md_file_))) \
	$(eval MAKEFILE_LIST :=  $$(lastword $$(MAKEFILE_LIST))) \
)
endef

###################################################
# Traverse all makefiles in directory
###################################################

define all-makefiles-under
$(wildcard $(1)/*/make.inc)
endef

define all-subdir-makefiles
$(call all-makefiles-under,$(call my-dir))
endef

###################################################
# Add target into ALL_TARGETS
###################################################

define add-target-into-build
$(eval ALL_TARGETS += $(strip $(1)))
endef

###################################################
# Build static library
###################################################

define build-static-library
	
endef

###################################################
# Download package
###################################################
WGET := wget --passive-ftp -nd -t 3 --no-check-certificate
GIT  := git  

###################################################
# Download package
# Argument 1 is the source location
# Argument 2 is the package name
# Argument 3 is the download methed
# Argument 3 is a space-separated list of optional argument
###################################################
define download-package
	$(Q)mkdir -p $($(2)_DL_DIR)	
endef





















