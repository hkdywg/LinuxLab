#!/bin/bash

set -e
# Establish qemu-system for arm64/arm32
#
# (C) 2023-03-09 hkdywg <hkdywg@163.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.


ROOT=${1%X}
QEMU_VERSION=${2%X}
QEMU_SRC=${3%X}
ARCH_CONFIG=${4%X}
OUTPUT=${ROOT}/output/config/qemu
WORKSPACE=${ROOT}/workspace
QEMU_WORKSPACE=${WORKSPACE}/qemu
MF=${WORKSPACE}/run_qemu.sh

CPU_CORE=`cat /proc/cpuinfo | grep processor | wc -l`


if [ ${ARCH_CONFIG} = "arm64" ];then
    # ARM 64-bit
    EMULATER=aarch64-softmmu,aarch64-linux-user
    CONFIG="--enable-kvm --enable-virtfs"
elif [ ${ARCH_CONFIG} = "arm" ];then
    # ARM 32-bit
    EMULATER=arm-softmmu,arm-linux-user
    CONFIG="--enable-virtfs --enable-kvm"
else
    echo -e "\033[31m Invalid arch config, not support yet! \033[0m"
    exit -1
fi
   

# config qemu and make makefile
cd ${QEMU_WORKSPACE}
./configure --target-list=${EMULATER} ${CONFIG}
make -j${CPU_CORE}



