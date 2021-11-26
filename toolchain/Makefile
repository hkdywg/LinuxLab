##################################################
#
# Cross compile toolchain
#
##################################################

# aarch64-linux-gnu for ARM64
AARCH64_GCC_NAME 		:= aarch64-linux-gnu
AARCH64_GCC_VERSION 	:= $(CONFIG_AARCH64_VERSION)
AARCH64_GCC_SRC 		:= $(CONFIG_TOOL_AARCH64_SRC)
AARCH64_GCC_SITE 		:= https://releases.linaro.org/components/toolchain/binaries/$(CONFIG_AARCH64_FULL_NAME)
AARCH64_GCC_GITHUB 		:= 
SUB_TARGET += aarch64-linux-gnu_download

# arm-linux-gnueabi for ARM32/Uboot
ARM_GNUEABI_GCC_NAME 	:= arm-linux-gnueabi
ARM_GNUEABI_GCC_VERSION := $(CONFIG_ARM_GNUEABI_VERSION)
ARM_GNUEABI_GCC_SRC 	:= $(CONFIG_TOOL_ARM_GNUEABI_SRC)
ARM_GNUEABI_GCC_SITE 	:= https://releases.linaro.org/components/toolchain/binaries/$(CONFIG_ARM_GNUEABI_FULL_NAME)
ARM_GNUEABI_GCC_GITHUB 	:=
SUB_TARGET += arm-linux-gnueabi_download

# arm-linux-gnueabihf for ARM32/Uboot
ARM_GNUEABIHF_GCC_NAME 	:= arm-linux-gnueabihf
ARM_GNUEABIHF_GCC_VERSION := $(CONFIG_ARM_GNUEABIHF_VERSION)
ARM_GNUEABIHF_GCC_SRC 	:= $(CONFIG_TOOL_ARM_GNUEABIHF_SRC)
ARM_GNUEABIHF_GCC_SITE 	:= https://releases.linaro.org/components/toolchain/binaries/$(CONFIG_ARM_GNUEABIHF_FULL_NAME)
ARM_GNUEABIHF_GCC_GITHUB 	:=
SUB_TARGET += arm-linux-gnueabihf_download
TOOLCHAIN_DOWNLOAD_SCRIPTS := $(srctree)/scripts/tool_chain/gnu.sh

AARCH64_TOOLCHAIN_DOWNLOAD_SH = $(CONFIG_SHELL) $(TOOLCHAIN_DOWNLOAD_SCRIPTS) $(srctree) $(AARCH64_GCC_NAME)X $(AARCH64_GCC_VERSION)X $(AARCH64_GCC_SRC)X $(AARCH64_GCC_SITE)X $(AARCH64_GCC_GITHUB)X 
ARMGNU_TOOLCHAIN_DOWNLOAD_SH = $(CONFIG_SHELL) $(TOOLCHAIN_DOWNLOAD_SCRIPTS) $(srctree) $(ARM_GNUEABI_GCC_NAME)X $(ARM_GNUEABI_GCC_VERSION)X $(ARM_GNUEABI_GCC_SRC)X $(ARM_GNUEABI_GCC_SITE)X $(ARM_GNUEABI_GCC_GITHUB)X 
ARMGNUHF_TOOLCHAIN_DOWNLOAD_SH = $(CONFIG_SHELL) $(TOOLCHAIN_DOWNLOAD_SCRIPTS) $(srctree) $(ARM_GNUEABIHF_GCC_NAME)X $(ARM_GNUEABIHF_GCC_VERSION)X $(ARM_GNUEABIHF_GCC_SRC)X $(ARM_GNUEABIHF_GCC_SITE)X $(ARM_GNUEABIHF_GCC_GITHUB)X 

#$(error ---- $(CONFIG_TOOL_ARM_GNUEABIHF_SRC)  $(ARM_GNUEABIHF_GCC_VERSION) )
aarch64-linux-gnu_download:
	@echo "prepare download toolchain"
	@$(AARCH64_TOOLCHAIN_DOWNLOAD_SH)


arm-linux-gnueabi_download:
	@echo "prepare download arm-linux-gnueabi toolchain"
	@$(ARMGNU_TOOLCHAIN_DOWNLOAD_SH)


arm-linux-gnueabihf_download:
	@echo "prepare download arm-linux-gnueabihf toolchain"
	@$(ARMGNUHF_TOOLCHAIN_DOWNLOAD_SH)

	
