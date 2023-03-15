LinuxLab
------------------------

## To Prepare

you need to install essential tooltain on host PC (such as "Ubuntu16.04"). Execute command:

```
sudo apt-get install -y qemu gcc make gdb git figlet
sudo apt-get install -y libncurses5-dev iasl wget
sudo apt-get install -y device-tree-compiler
sudo apt-get install -y flex bison libssl-dev libglib2.0-dev
sudo apt-get install -y libfdt-dev libpixman-1-dev
sudo apt-get install -y python pkg-config u-boot-tools intltool xsltproc
sudo apt-get install -y gperf libglib2.0-dev libgirepository1.0-dev
sudo apt-get install -y gobject-introspection
sudo apt-get install -y python2.7-dev python-dev bridge-utils
sudo apt-get install -y uml-utilities net-tools
sudo apt-get install -y libattr1-dev libcap-dev
sudo apt-get install -y kpartx libsdl2-dev libsdl1.2-dev
sudo apt-get install -y debootstrap bsdtar sudo apt-get install -y libelf-dev gcc-multilib g++-multilib
sudo apt-get install -y libcap-ng-dev
sudo apt-get install -y libmount-dev libselinux1-dev libffi-dev libpulse-dev
sudo apt-get install -y liblzma-dev python-serial
sudo apt-get install -y gtk+-2.0 glib-2.0 libglade2-dev


On 64-Bit Machine:

sudo apt-get install lib32z1 lib32z1-de libc:i386

```


**NOTE!**

If you first install or user git, please configure git as follow

```
git config --global user.name "Your Name"
git config --global user.email "Your Email"
```

## To Start

First of all, you need to obtain source code of LinuxLab from GitHub, follow these steps to get newest and stable branch.

```
git clone https://github.com/hkdywg/LinuxLab.git
```

The next step, you need to build LinuxLab with common Kbuild syntax.The Kbuild will help you easily to build all software and kernel.
So utilise command on your terminal:

```
cd */LinuxLab
make defconfig
make
cd workspace
./run_qemu.sh
```

Then, the LinuxLab will auto-compile and generate a Distro-Linux, more userful information will be generate.
