##
## benchmark/build_core/definition.mk
##
## History:
##    2024/04/12 - [yinwg] Created file
##
## Copyright (C) 2016, JiangSu New Vision Automotive CO.,LTD.
## 
## All rights reserved. This software is New Vision property. Duplication
## or disclosure without New Vision written authorization is prohibited.
##

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











