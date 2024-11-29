##
## make.inc
##
## History:
##    2024/11/29 - [yinwg] Created file
##
## 	This program is free software; you can redistribute it and/r modify
##  it under the terms of the GNU General Public License version 2 as
##  published by the Free Software Foundation.
##

SUB_TARGET += lockdep-build

# This is a -R option in MAKEFLAGS in the top-level Makefile
# The -R option affects the use of COMPILE.c variables in kexec compile
MAKEFLAGS := 

lockdep-build:
	cat $(srctree)/package/lockdep/lockdep/config >> $(KERNEL_KCONFIG_FILE); \
	make kernel-build

