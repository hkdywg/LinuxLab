#!/bin/bash

set -e
# generate run_qemu.sh scripts
#
# (C) 2023-03-15 hkdywg <hkdywg@163.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.

ROOT=${1%X}
ARCH_CONFIG=${2%X}
SYS_RAM_SIZE=${3%X}
CORE_TYPE=${4%X}
CORE_NUM=${5%X}
WORKSPACE=${ROOT}/workspace
QEMU_WORKSPACE=${WORKSPACE}/qemu
MF=${WORKSPACE}/run_qemu.sh

cat << EOF > ${MF} 
#!/bin/bash

# Build system.
#
# (C) 2023.03.15 hkdywg <hkdywg@163.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.

EOF


if [ ${ARCH_CONFIG} = "arm64" ];then
    # ARM 64-bit
    echo -e "QEMU=${QEMU_WORKSPACE}/build/aarch64-softmmu/qemu-system-aarch64" >> ${MF}
    echo -e "KERNEL_IMAGE=${WORKSPACE}/kernel/build_out/arch/arm64/boot/Image" >> ${MF}
elif [ ${ARCH_CONFIG} = "arm" ];then
    # ARM 32-bit
    echo -e "QEMU=${QEMU_WORKSPACE}/arm-softmmu/qemu-system-arm" >> ${MF}
    echo -e "KERNEL_IMAGE=${WORKSPACE}/kernel/build_out/arch/arm/boot/Image" >> ${MF}
else
    echo -e "\033[31m Invalid arch config, not support yet! \033[0m"
    exit -1
fi

echo -e "ROOT_DIR=${WORKSPACE}/rootfs" >> ${MF}
echo -e "RAM_SIZE=${SYS_RAM_SIZE}" >> ${MF}
echo -e "CORE_TYPE=${CORE_TYPE}" >> ${MF}
echo -e "CORE_NUM=${CORE_NUM}" >> ${MF}

cat << EOF >> ${MF}
CMDLINE="earlycon root=/dev/vda rw rootfstype=ext4 console=ttyAMA0 init=/linuxrc loglevel=8"
sudo \${QEMU} \\
    -M virt \\
    -m \${RAM_SIZE} \\
    -cpu \${CORE_TYPE} \\
    -smp \${CORE_NUM} \\
    -kernel \${KERNEL_IMAGE} \\
    -device virtio-blk-device,drive=hd0 \\
    -drive if=none,file=\${ROOT_DIR}/rootfs.img,format=raw,id=hd0 \\
    -nographic \\
    -append "\${CMDLINE}"
EOF

chmod +x ${MF}


