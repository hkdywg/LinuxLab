##############################################################################
#
# uboot
#
##############################################################################

UBOOT_NAME 			:= u-boot
UBOOT_VERSION 		:= $(CONFIG_UBOOT_VERSION) 
UBOOT_SITE 			:= $(CONFIG_UBOOT_WGET_SITE)
UBOOT_GITHUB 		:= $(CONFIG_UBOOT_GITHUB_SITE)
UBOOT_CROSS_COMPILE := $(CONFIG_UBOOT_CROSS_COMPILE)
UBOOT_PATCH 		:= $(srctree)/boot/u-boot/patch
UBOOT_OUT 			:=
UBOOT_SCRIPTS		:= $(srctree)/scripts/boot/uboot.sh

SUB_TARGET 	+= uboot-download

UBOOT_SH = $(CONFIG_SHELL) $(UBOOT_SCRIPTS) \
		   ${1}X ${UBOOT_NAME}X ${UBOOT_VERSION} ${UBOOT_SITE}X \
		   ${UBOOT_GITHUB}X ${CONFIG_UBOOT_SRC}X ${CONFIG_ARCH}X ${UBOOT_CROSS_COMPILE}X 

$(warning $(UBOOT_SH))

uboot-download: 
	${Q}$(call UBOOT_SH,${srctree})


