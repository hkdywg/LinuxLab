# LinuxLab

<p align="center">
  <img src="https://img.shields.io/badge/LinuxLab-Project-blue?style=for-the-badge&logo=linux" alt="LinuxLab">
  <img src="https://img.shields.io/badge/License-MIT-green?style=for-the-badge" alt="License">
</p>

---

## 📋 概述

LinuxLab 是一个用于学习和实验 Linux 内核的虚拟化环境，基于 QEMU 构建，提供了一个便捷的嵌入式 Linux 开发平台。

---

## 🔧 环境准备

### 安装依赖工具

在主机上执行以下命令安装必要的工具（如 Ubuntu 16.04+）：

```bash
# 基础开发工具
sudo apt-get install -y qemu gcc make gdb git figlet

# 编译工具链
sudo apt-get install -y libncurses5-dev iasl wget
sudo apt-get install -y device-tree-compiler
sudo apt-get install -y flex bison libssl-dev libglib2.0-dev
sudo apt-get install -y libfdt-dev libpixman-1-dev

# Python 相关
sudo apt-get install -y python pkg-config u-boot-tools intltool xsltproc
sudo apt-get install -y python2.7-dev python-dev

# 网络与系统工具
sudo apt-get install -y gobject-introspection
sudo apt-get install -y bridge-utils uml-utilities net-tools

# 库依赖
sudo apt-get install -y libattr1-dev libcap-dev
sudo apt-get install -y kpartx libsdl2-dev libsdl1.2-dev
sudo apt-get install -y debootstrap bsdtar sudo apt-get install -y libelf-dev gcc-multilib g++-multilib
sudo apt-get install -y libcap-ng-dev
sudo apt-get install -y libmount-dev libselinux1-dev libffi-dev libpulse-dev
sudo apt-get install -y liblzma-dev python-serial

# 图形界面支持
sudo apt-get install -y gtk+-2.0 glib-2.0 libglade2-dev

# 构建工具
sudo apt-get install -y ninja-build
```

### 64 位系统额外依赖

```bash
sudo apt-get install -y lib32z1 lib32z1-de libc6:i386
```

---

## ⚙️ Git 配置

如果是首次使用 Git，请先配置用户信息：

```bash
git config --global user.name "Your Name"
git config --global user.email "your.email@example.com"
```

---

## 🚀 快速开始

### 1. 克隆代码仓库

```bash
git clone https://github.com/hkdywg/LinuxLab.git
```

### 2. 编译项目

使用 Kbuild 构建系统编译 LinuxLab：

```bash
cd LinuxLab
make defconfig  # 加载默认配置
make            # 开始编译
```

### 3. 运行 QEMU

```bash
cd workspace
./run_qemu.sh
```

---

## 📖 更多文档

编译完成后，系统会自动生成有用的使用信息和文档。

---

<p align="center">祝您使用愉快！🎉</p>
