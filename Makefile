#
# Global Makefile
# Author: hkdywg <hkdywg@163.com>
#
# Copyright (C) 2021 LinuxLab
#
#

VERSION = 1
PATCHLEVEL = 0
SUBLEVEL = 1

MAKEFLAGS += -rR --no-print-directory

# Use `make V=1` to see full commands

ifeq ("$(origin V)", "command line")
	KBUILD_VERBOSE = $(V)
endif


# Default target
PHONY := _all
_all:

# if want to locate output file in a separate directory, can use `make O=outputdir`
ifeq ("$(origin O)", "command line")
	KBUILD_OUTPUT := $(O)
endif

ifneq ($(KBUILD_OUTPUT),)
saved-output := $(KBUILD_OUTPUT)
endif

srctree 	:= $(CURDIR)
objtree 	:= $(CURDIR)
src 		:= $(srctree)
obj 		:= $(objtree)

VPATH := $(srctree)

export srctree objtree VPATH

KCONFIG_CONFIG ?= .config
export KCONFIG_CONFIG

# SHELL used by kbuild
CONFIG_SHELL := $(shell if [ -x "$$BASH" ];then echo $$BASH;\
	else if [ -x /bin/bash ];then echo /bin/bash; \
	else echo sh; fi; fi)

# Host make variables ...
HOSTCC 		= gcc
HOSTCXX 	= g++
HOSTCFLAGS 	= -Wall -Wmissing-prototypes -Wstrict-prototypes -O2 -fomit-frame-pointer
HOSTCXXFLAGS = -O2

# Crosscompile variables ...

CC 			= $(CROSS_COMPILE)gcc
AS 			= $(CROSS_COMPILE)as
LD 			= $(CROSS_COMPILE)ld
CPP 		= $(CC) -E
AR 			= $(CROSS_COMPILE)ar
NM 			= $(CROSS_COMPILE)nm
STRIP		= $(CROSS_COMPILE)strip
OBJCOPY 	= $(CROSS_COMPILE)objcopy
OBJDUMP 	= $(CROSS_COMPILE)objdump
AWK			= awk
INSTALLKERNEL 	:= installkernel
PERL 		= perl

CHECKFLAGS 	:= -D__linux__ -Dlinux -D__STDC__ -Dunix -D__unix__ \
				-Wbitwise -Wno-return-void $(CF)
CFLAGS_KERNEL 	=
AFLAGS_KERNEL 	=

LINUXINCLUDE 	:= -Iinclude \
					$(if $(KBUILD_SRC), -I$(srctree)/include) \
					-include include/generated/autoconf.h

KBUILD_CPPFLAGS 	:= -D__KERNEL__

KBUILD_CFLAGS 	:= -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs \
					-fno-strict-aliasing -fno-common \
					-Werror-implicit-function-declaration \
					-Wno-format-security \
					-fno-delete-null-pointer-checks

KBUILD_AFLAGS_KERNEL :=
KBUILD_CFLAGS_KERNEL :=
KBUILD_AFLAGS := -D_ASSEMBLY__

KBUILD_DEFCONFIG = defconfig

export ARCH SRCARCH CONFIG_SHELL HOSTCC HOSTCFLAGS CROSS_COMPILE AS LD CC
export CPP AR NM STRIP OBJCOPY OBJDUMP
export MAKE AWK GENKSYMS INSTALLKERNEL PERL UTS_MACHINE
export HOSTCXX HOSTCXXFLAGS

export KBUILD_CPPFLAGS NOSTDINC_FLAGS LINUXINCLUDE OBJCOPYFLAGS LDFLAGS
export KBUILD_CFLAGS CFLAGS_KERNEL
export KBUILD_AFLAGS AFLAGS_KERNEL
export KBUILD_AFLAGS_KERNEL KBUILD_CFLAGS_KERNEL
export KBUILD_ARFLAGS

ifeq ($(KBUILD_VERBOSE),1)
	quiet =
	Q = 
else
	quiet = quiet_
	Q = @
endif

export quiet Q KBUILD_VERBOSE

# Look for make include files relative to root fo kernel src
MAKEFLAGS += --include-dir=$(srctree)

-include include/config/auto.conf

include $(srctree)/scripts/Kbuild.include

PHONY += FORCE

FORCE:

PHONY += outputmakefile 
outputmakefile:
ifneq ($(KBUILD_SRC),)
	$(Q)ln -fsn $(srctree) source
	$(Q)$(CONFIG_SHELL) $(srctree)/scripts/mkmakefile \
		$(srctree) $(objtree) $(VERSION) $(PATCHLEVEL)
endif

RCS_FIND_IGNORE := \( -name SCCS -o -name BitKeeper -o -name .svn -o -name CVS -o -name .pc -o -name .hg -o -name .git \) -prune -o
# Basic helpers built in scripts/
PHONY += scripts_basic
scripts_basic:
	$(Q)$(MAKE) $(build)=scripts/basic

scripts/basic/%: scripts_basic ;


export KBUILD_DEFCONFIG KBUILD_KCONFIG

SUB_TARGET := 

export SUB_TARGET

%config: scripts_basic outputmakefile FORCE
	$(Q)mkdir -p include/linux include/config
	$(Q)$(MAKE) $(build)=scripts/kconfig $@

include $(srctree)/$(KCONFIG_CONFIG)

_all:all


# Toolchain
ifdef CONFIG_TOOLCHAIN
include toolchain/Makefile
endif

# U-boot
ifdef CONFIG_UBOOT
include boot/Makefile
endif

# Kernel
include kernel/Makefile

# RootFs
ifdef CONFIG_ROOTFS
include fs/Makefile
endif

# The all:target is the default when no target is given on the command line
all: $(SUB_TARGET)
	@echo "build default target: $(SUB_TARGET)"

PHONY += help

help:
	@echo "srctree is $(srctree)"
	@echo "CONFIG_SHELL is $(CONFIG_SHELL)"
