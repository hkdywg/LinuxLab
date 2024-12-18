#!/bin/sh

dir=/lib/modules/4.14.70-ltsi-yocto-standard/usb_dev_mod
echo "starting usb stroge device driver"

insmod $dir/phy-generic.ko
insmod $dir/usb-common.ko
insmod $dir/usbcore.ko
insmod $dir/usbhid.ko
insmod $dir/ehci-hcd.ko
insmod $dir/ehci-platform.ko
insmod $dir/xhci-hcd.ko
insmod $dir/xhci-plat-hcd.ko
insmod $dir/scsi_mod.ko
insmod $dir/usb-storage.ko
insmod $dir/scsi_transport_sas.ko
insmod $dir/sd_mod.ko
insmod $dir/libsas.ko

