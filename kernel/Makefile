##################################################################
#
# Linux kernel
# ##################################################################

LINUX_KERNEL_NAME 			:= linux
LINUX_KERNEL_VERSION 		:= $(CONFIG_LINUX_KERNEL_VERSION)
LINUX_KERNEL_SITE 			:= $(CONFIG_LINUX_KERNEL_WGET_SITE)
LINUX_KERNEL_GITHUB 		:= $(CONFIG_LINUX_KERNEL_GITHUB_SITE)
LINUX_KERNEL_SCRIPTS 		:= $(srctree)/scripts/kernel/linux.sh

SUB_TARGET 					+= kernel_download

LINUX_KERNEL_SH 	= $(CONFIG_SHELL) ${LINUX_KERNEL_SCRIPTS} $(1)X $(LINUX_KERNEL_NAME)X \
					  $(LINUX_KERNEL_VERSION)X	$(LINUX_KERNEL_SITE)X ${LINUX_KERNEL_GITHUB}X $(CONFIG_LINUX_KERNEL_SRC)X

kernel_download:
		$(Q)$(call LINUX_KERNEL_SH, $(srctree))
