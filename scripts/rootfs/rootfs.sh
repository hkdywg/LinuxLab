#!/bin/bash

set -e

# Establish rootfs.
#
# (C) 2023.03.14 hkdywg <hkdywg@163.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.

# root dir 
ROOTFS_WORK_DIR=${1%X}

# rootfs name
ROOTFS_NAME=${2%X}

# rootfs version
ROOTFS_VERSION=${3%X}

# rootfs size (MB)
ROOTFS_SIZE=${4%X}
[ ! ${ROOTFS_SIZE} ] && ROOTFS_SIZE=512

# cross-compile path
CROSS_COMPILE_TOOL_PATH=${5%X}

# busybox install path
BUSYBOX_BUILD_OUTPATH=${6%X}

# rootfs path
ROOTFS_PATH=${ROOTFS_WORK_DIR}/rootfs
mkdir -p ${ROOTFS_PATH}

# rootfs type tools
FS_TYPE=ext4
FS_TYPE_TOOL=mkfs.ext4

# copy library
copy_libs() {
    for lib in $1/*.so*; do
        if [[ -e "$2/$(basename $lib)" ]];then
            : # skip, continue
        elif [[ -L "$lib" ]];then
            ln -s $(basename $(readlink $lib)) $2/$(basename $lib)
        else
            cp -a $lib $2/$(basename $lib)
        fi
    done
}

# prepare direct on rootfs
sudo rm -rf ${ROOTFS_PATH}/*
[ -d ${BUSYBOX_BUILD_OUTPATH}/_install/ ] && cp ${BUSYBOX_BUILD_OUTPATH}/_install/* ${ROOTFS_PATH} -raf
mkdir -p ${ROOTFS_PATH}/proc
mkdir -p ${ROOTFS_PATH}/sys
mkdir -p ${ROOTFS_PATH}/tmp
mkdir -p ${ROOTFS_PATH}/root
mkdir -p ${ROOTFS_PATH}/var
mkdir -p ${ROOTFS_PATH}/mnt
mkdir -p ${ROOTFS_PATH}/etc/init.d


# rcS
RC=${ROOTFS_PATH}/etc/init.d/rcS

# auto create rcS file
cat << EOF > ${RC}
PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin:
SHELL=/bin/sh
export PATH SHELL
/bin/mount -a
/bin/hostname ywg_rootfs
[ -f /bin/busybox ] && chmod 7755 /bin/busybox

# networking
ifconfig lo up > /dev/null 2>&1
ifconfig lo 127.0.0.1
ifconfig eth0 up > /dev/null 2>&1
ifconfig 172.29.4.44
route add default gw 172.29.4.1

mkdir -p /dev/pts
mount -t devpts devpts /dev/pts
echo /sbin/mdev > /proc/sys/kernel/hotplug
mdev -s
/usr/sbin/telnetd &

echo "welcome to ywg_rootfs"
EOF
chmod 755 ${RC}

# fstab
FSTAB=${ROOTFS_PATH}/etc/fstab
# auto create fstab file
cat << EOF > ${FSTAB}
proc /proc proc defaults 0 0
tmpfs /tmp tmpfs defaults 0 0
sysfs /sys sysfs defaults 0 0
tmpfs /dev devfs defaults 0 0
debugfs /sys/kernel/debug debugfs defaults 0 0
EOF

# inittab
INITTAB=${ROOTFS_PATH}/etc/inittab
# auto create inittab file
cat << EOF > ${INITTAB}
::sysinit:/etc/init.d/rcS
::respawn:-/bin/sh
::askfirst:-/bin/sh
::ctrlaltdel:/sbin/reboot
::shutdown:/bin/umount -a -r
EOF

# user group
GROUP=${ROOTFS_PATH}/etc/group
# auto create group file
cat << EOF > ${GROUP}
root::0:0:root:/:/bin/sh
EOF

# dns 
DNS=${ROOTFS_PATH}/etc/resolv.conf
# auto create dns file
cat << EOF > ${DNS}
nameserver 1.2.4.8
nameserver 8.8.8.8
EOF

# hosts
HOSTS=${ROOTFS_PATH}/etc/hosts
# auto create hosts file
cat << EOF > ${HOSTS}
127.0.0.1 localhosts
EOF


# install cross-compiler library
# ---> linux 2.6 not support library
mkdir -p ${ROOTFS_PATH}/lib

copy_libs ${CROSS_COMPILE_TOOL_PATH}/libc/lib ${ROOTFS_PATH}/lib

# create subdev 
mkdir -p ${ROOTFS_PATH}/dev
sudo mknod ${ROOTFS_PATH}/dev/tty1 c 4 1
sudo mknod ${ROOTFS_PATH}/dev/tty2 c 4 2
sudo mknod ${ROOTFS_PATH}/dev/tty3 c 4 3
sudo mknod ${ROOTFS_PATH}/dev/tty4 c 4 4
sudo mknod ${ROOTFS_PATH}/dev/console c 5 1
sudo mknod ${ROOTFS_PATH}/dev/null c 1 3

# change root
sudo chown root.root ${ROOTFS_PATH}/* -R

dd if=/dev/zero of=${ROOTFS_WORK_DIR}/rootfs.img bs=1M count=${ROOTFS_SIZE}
${FS_TYPE_TOOL} -F ${ROOTFS_WORK_DIR}/rootfs.img
mkdir -p ${ROOTFS_WORK_DIR}/tmpfs
sudo mount -t ${FS_TYPE} ${ROOTFS_WORK_DIR}/rootfs.img ${ROOTFS_WORK_DIR}/tmpfs -o loop
sudo cp -raf ${ROOTFS_PATH}/* ${ROOTFS_WORK_DIR}/tmpfs
sudo cp -raf ${ROOTFS_WORK_DIR}/user_install/* ${ROOTFS_WORK_DIR}/tmpfs/
sync
sudo umount ${ROOTFS_WORK_DIR}/tmpfs









