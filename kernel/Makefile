##################################################################
#
# Linux kernel
# ##################################################################

LINUX_KERNEL_NAME 			:= linux
LINUX_KERNEL_VERSION 		:= $(CONFIG_LINUX_KERNEL_VERSION)
LINUX_KERNEL_SITE 			:= $(CONFIG_LINUX_KERNEL_WGET_SITE)
LINUX_KERNEL_GITHUB 		:= $(CONFIG_LINUX_KERNEL_GITHUB_SITE)
LINUX_KERNEL_SCRIPTS 		:= $(srctree)/scripts/kernel/linux.sh

KERNEL_WORKSPACE 			:= $(srctree)/workspace/kernel
KERNEL_BUILD_OUTPATH 		:= $(KERNEL_WORKSPACE)/build_out
KERNEL_KCONFIG_FILE 		:= $(KERNEL_BUILD_OUTPATH)/.config
KERNEL_IMAGE 				:= $(KERNEL_BUILD_OUTPATH)/arch/arm64/boot/Image

export KERNEL_KCONFIG_FILE
export KERNEL_IMAGE

SUB_TARGET 					+= kernel-download kernel-defconfig kernel-build

LINUX_KERNEL_SH 	= $(CONFIG_SHELL) ${LINUX_KERNEL_SCRIPTS} $(1)X $(LINUX_KERNEL_NAME)X \
					  $(LINUX_KERNEL_VERSION)X	$(LINUX_KERNEL_SITE)X ${LINUX_KERNEL_GITHUB}X $(CONFIG_LINUX_KERNEL_SRC)X

PHONY := 

kernel-download:
		$(Q)$(call LINUX_KERNEL_SH, $(srctree))

kernel-menuconfig:
	@mkdir -p $(KERNEL_BUILD_OUTPATH)
	${Q}${MAKE} -C $(KERNEL_WORKSPACE) ARCH=$(ARCH) menuconfig O=$(KERNEL_BUILD_OUTPATH) 	

kernel-defconfig:
	@mkdir -p $(KERNEL_BUILD_OUTPATH)
	${Q}${MAKE} -C $(KERNEL_WORKSPACE) ARCH=$(ARCH) $(CONFIG_KERNEL_DEFCONFIG) O=$(KERNEL_BUILD_OUTPATH)

kernel-clean:
	@${Q}${MAKE} -C $(KERNEL_WORKSPACE) clean O=$(KERNEL_BUILD_OUTPATH)


CPU_CORE=$(shell cat /proc/cpuinfo | grep processor | wc -l)

ifeq ($(CONFIG_KERNEL_CROSS_COMPILE),"arm-linux-gnueabi")
TOOL_CHAIN_NAME := $(basename $(basename $(notdir $(CONFIG_ARM_GNUEABI_FULL_NAME))))
CROSS_COMPILE_CONFIG := $(srctree)/output/toolchain/$(TOOL_CHAIN_NAME)/bin/arm-linux-gnueabi-
endif
ifeq ($(CONFIG_KERNEL_CROSS_COMPILE),"arm-linux-gnueabihf")
TOOL_CHAIN_NAME := $(basename $(basename $(notdir $(CONFIG_ARM_GNUEABIHF_FULL_NAME))))
CROSS_COMPILE_CONFIG := $(srctree)/output/toolchain/$(TOOL_CHAIN_NAME)/bin/arm-linux-gnueabihf-
endif
ifeq ($(CONFIG_KERNEL_CROSS_COMPILE),"aarch64-linux-gnu")
TOOL_CHAIN_NAME := $(basename $(basename $(notdir $(CONFIG_AARCH64_FULL_NAME))))
CROSS_COMPILE_CONFIG := $(srctree)/output/toolchain/$(TOOL_CHAIN_NAME)/bin/aarch64-linux-gnu-
endif

kernel-build:$(KERNEL_KCONFIG_FILE)
	@mkdir -p $(KERNEL_BUILD_OUTPATH)
	${Q}${MAKE} -C $(KERNEL_WORKSPACE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE_CONFIG) O=$(KERNEL_BUILD_OUTPATH) -j$(CPU_CORE)

$(KERNEL_KCONFIG_FILE):
	@echo >&2 "\033[31m***\033[0m"
	@echo >&2 "\033[31m*** $(KERNEL_KCONFIG_FILE) is not found!\033[0m"
	@echo >&2 "\033[31m***\033[0m"
	@echo >&2 "\033[31m*** please run some configurator(e.g. "make uboot-menuconfig")\033[0m"
	@echo >&2 "\033[31m***\033[0m"
	@/bin/false
