#!/bin/bash

ROOT=${1%X}
QEMU_VERSION=${2%X}
QEMU_SRC=${3%X}
QEMU_WGET_SITE=${4%X}
QEMU_GITHUB_SITE=${5%X}

OUTPUT=${ROOT}/output
WORKSPACE=${ROOT}/workspace

[ ! -d ${OUTPUT} ] && mkdir -p ${OUTPUT}

if [ ${QEMU_SRC} = "1" ]; then
  cd ${ROOT}/dl
  git clone ${QEMU_GITHUB_SITE}
  sudo apt-get install -y libglib2.0-dev libfdt-dev
  sudo apt-get install -y libpixman-1-dev
  cp -raf ${ROOT}/dl/qemu ${OUTPUT}
  cd ${OUTPUT}/qemu
  git tag > TAG_LIST
  git reset --hard ${QEMU_VERSION}
  if [ $? -ne 0 ]; then
      cat TAG_LIST
      echo "\033[31m qemu only support obove version \033[0m"
      exit -1
  fi
  ln -s ${OUTPUT}/qemu ${WORKSPACE}/qemu
fi

if [ ${QEMU_SRC} = "2" ];then
    QEMU_WGET_NAME=qemu-${QEMU_VERSION}.tar.xz
    if [ ! -f ${QEMU_WGET_NAME} ]; then
        cd ${ROOT}/dl/
        wget ${QEMU_WGET_SITE}/${QEMU_WGET_NAME}
    fi
    cp ${QEMU_WGET_NAME} ${OUTPUT}
    cd ${OUTPUT}
    tar -xJf ${QEMU_WGET_NAME}
    rm ${QEMU_WGET_NAME}
    ln -s ${OUTPUT}/qemu-${QEMU_VERSION} ${WORKSPACE}/qemu
fi
