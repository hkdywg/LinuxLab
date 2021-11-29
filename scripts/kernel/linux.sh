#!/bin/bash

set -e

ROOT=${1%X}
LINUX_KERNEL_NAME=${2%X}
LINUX_KERNEL_VERSION=${3%X}
LINUX_KERNEL_SITE=${4%X}
LINUX_KERNEL_GITHUB=${5%X}
LINUX_KERNEL_SRC=${6%X}

OUTPUT=${ROOT}/output/kernel

case ${LINUX_KERNEL_SRC} in
    # Get from github
    1)
        if [ ! -d ${ROOT}/dl/kernel ];then
            cd ${ROOT}/dl
            git clone ${LINUX_KERNEL_GITHUB} --depth=1 --branch v${LINUX_KERNEL_VERSION}
        else
            git pull
        fi
        mkdir -p ${OUTPUT}/${LINUX_KERNEL_NAME}
        cp ${ROOT}/dl/kernel ${OUTPUT}/${LINUX_KERNEL_NAME}
        rm ${ROOT}/dl/kernel -rf
        ;;
    2)
        echo -e "\033[31m this method is not support!!!\033[0m"
        exit 0
        ;;

    3)
        BASE_NAME=${LINUX_KERNEL_NAME}-${LINUX_KERNEL_VERSION}.tar.xz
        BASE=${LINUX_KERNEL_NAME}-${LINUX_KERNEL_VERSION}
        if [ ! -f ${ROOT}/dl/${BASE_NAME} ]; then
            cd ${ROOT}/dl
            wget ${LINUX_KERNEL_SITE}/${BASE_NAME}
        fi
        mkdir -p ${OUTPUT}/${LINUX_KERNEL_NAME}
        cp ${ROOT}/dl/${BASE_NAME} ${OUTPUT}/${LINUX_KERNEL_NAME}
        cd ${OUTPUT}/${LINUX_KERNEL_NAME}
        tar -xf ${BASE_NAME}
        if [ $? -ne 0 ];then
            echo -e "\033[31m Invalid tar operation \033[0m"
            exit -1
        fi
        echo ${LINUX_KERNEL_VERSION} > ${OUTPUT}/${LINUX_KERNEL_NAME}/version
        echo -e "\033[32m linux kernel download successed!! \033[0m"
        ;;
esac

