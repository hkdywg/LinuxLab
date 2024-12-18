#!/bin/sh
modprobe -a pvrsrvkm uvcs_drv vsp2 vspm vspm_if mmngrbuf mmngr
insmod /lib/modules/5.10.0-yocto-standard/extra/uio_imr.ko
ln -sf /dev/uio0 /dev/imr0
