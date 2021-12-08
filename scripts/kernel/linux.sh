#!/bin/bash

set -e

ROOT=${1%X}
LINUX_KERNEL_NAME=${2%X}
LINUX_KERNEL_VERSION=${3%X}
LINUX_KERNEL_SITE=${4%X}
LINUX_KERNEL_GITHUB=${5%X}
LINUX_KERNEL_SRC=${6%X}

OUTPUT=${ROOT}/output/kernel
WORKSPACE=${ROOT}/workspace

[ ! -d ${WORKSPACE} ] && mkdir -p ${WORKSPACE}
[ ! -d ${OUTPUT} ] && mkdir -p ${OUTPUT}

case ${LINUX_KERNEL_SRC} in
    # Get from github
    1)
        cd ${ROOT}/dl
        git clone ${LINUX_KERNEL_GITHUB} --depth=1 --branch v${LINUX_KERNEL_VERSION}
        cp ${ROOT}/dl/${LINUX_KERNEL_NAME} ${OUTPUT}/ -rf
        rm ${ROOT}/dl/${LINUX_KERNEL_NAME} -rf
        echo -e "\033[32m linux kernel download successed!! \033[0m"
        ln -s ${OUTPUT}/${LINUX_KERNEL_NAME} ${WORKSPACE}/kernel
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
        mkdir -p ${OUTPUT}/
        cp ${ROOT}/dl/${BASE_NAME} ${OUTPUT}
        cd ${OUTPUT}
        tar -xf ${BASE_NAME}
        if [ $? -ne 0 ];then
            echo -e "\033[31m Invalid tar operation \033[0m"
            exit -1
        fi
        echo -e "\033[32m linux kernel download successed!! \033[0m"
        ln -s ${OUTPUT}/${BASE} ${WORKSPACE}/kernel
        ;;
esac

