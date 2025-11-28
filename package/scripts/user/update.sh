#!/bin/sh

PROJECT_ARRAY=
HUD_PROJECT=

function print_info()
{
    printf "\e[1;32mControl_Box update info:%s\e[0m\n" "$1"
}

function print_err()
{
    printf "\e[1;31mControl_Box update Error:%s\e[0m\n" "$1"
}

function get_available_project()
{
    PROJECT_ARRAY=$(find /project -mindepth 1 -maxdepth 1 -not -path "/project/libkms" -type d -print ! -name "." | awk -F/ '{print $NF}' | sort -t '-' -k2,2)
    local len
    len=$(echo $PROJECT_ARRAY | tr -cd ' ' | wc -c)
    PROJECT_ARRAY_LEN=$(($len+1))
    #print_info ${PROJECT_ARRAY}
}

function choose_project()
{
    echo "Select a target project"

    echo ${PROJECT_ARRAY} | xargs -n 1 | sed "=" | sed "N;s/\n/. /"

    local index
    read -p "Which would you select: " index

    if [ -z $index ]; then
	print_err "Nothing select"
	exit 0
    fi

    if [[ -n $index && $index =~ ^[0-9]+$ && $index -ge 1 && $index -le $PROJECT_ARRAY_LEN ]]; then
	#set -- $PROJECT_ARRAY
	#HUD_PROJECT=$(eval echo \$$index)
	HUD_PROJECT=$(echo $PROJECT_ARRAY | awk -v field="$index" '{print $field}')
	#echo $HUD_PROJECT
    else
	print_err "Invalid input!"
	exit 1
    fi
}

function update_usage()
{
    echo "Usage: "
    echo ""$0"			- Show this menu"
    echo ""$0" lunch		- Select a project to update"
    echo ""$0" [project]	- Update [project] directly"
}

get_available_project

if [  $# -eq 1 ]; then
    if [ "$1" = "lunch" ]; then
	choose_project || exit 0
    else
	target=$1
	found=0
	#set -- $PROJECT_ARRAY
	for word in $PROJECT_ARRAY; do
	    if [ "$word" = "$target" ]; then
		HUD_PROJECT=$target
		found=1
		break
	    fi
	done
	if [ $found -eq 0 ]; then
	    print_err "input project is invalid, not support yet!"
	    exit 1
	fi
    fi
else
    update_usage && exit 0
fi

if grep -q "/dev/mmcblk0p1" /proc/mounts; then
    umount /dev/mmcblk0p1
fi

print_info "Start $HUD_PROJECT project update"

##################################################################
# Step 1: format emmc
##################################################################

print_info "Erase all data, format emmc"
# format mmcblk to delete mmc partition information
# if mmc has partition information, need to select 'y' to format
mkfs.ext4 /dev/mmcblk0 > /dev/null 2>&1 << EOF
y	
EOF

sleep 0.2

# create a new partition      
# set new partition to primary           
# set partition number to 1                               
# use default start sectors                         
# use default end sectors            
# write to partition table, and quite

fdisk /dev/mmcblk0 > /dev/null 2>&1 << EOF
n
p
1


w
EOF

if [ $? -eq 0 ]; then
print_info "create partition successed!"
else
print_err "create partition failed, exit update process!"
exit 1
fi

sleep 0.2

print_info "format system partition"

mkfs.ext4 /dev/mmcblk0p1 > /dev/null 2>&1 << EOF
y	
EOF

sleep 0.2
##################################################################
# Step 2: mount emmc partition, prepare to copy system files 
##################################################################
mount /dev/mmcblk0p1 /mnt/disk

if [ $? -eq 0 ]; then
print_info "mount partition successed!"
else
print_err "mount partition failed, exit update process!"
exit 1
fi

##################################################################
# Step 3: copy base rootfs files 
##################################################################

if [ -d /project/$HUD_PROJECT ]; then
# unzip base rootfs files to mmc partition
tar -xzf /project/base_rootfs.tar.gz -C /mnt/disk/		
sync
else
print_err "$HUD_PROJECT project files is not exist, update failed!"
exit 1
fi

print_info "base rootfs files write done!"

##################################################################
# Step 4: copy project relation system files 
##################################################################

cp /project/$HUD_PROJECT/Image /mnt/disk/boot/Image_test
cp /project/$HUD_PROJECT/r8a77990-ebisu.dtb /mnt/disk/boot/r8a77990_test.dtb
cp /project/$HUD_PROJECT/rcS /mnt/disk/etc/init.d/rcS
cp /project/libkms/* /mnt/disk/usr/lib/
sync

print_info "project relation system files write done!"

##################################################################
# Step 5: copy HUD application 
##################################################################

if [ -f /project/$HUD_PROJECT/HUD.tar.gz ]; then
   tar -xzf /project/$HUD_PROJECT/HUD.tar.gz -C /mnt/disk/ 
elif [  -f /project/$HUD_PROJECT/HUD.tar.xz ]; then
   tar -xzf /project/$HUD_PROJECT/HUD.tar.xz -C /mnt/disk/ 
else
   print_err "HUD project files is not exist, stop this update process!"
fi

if [ $? -ne 0 ]; then
print_err "unzip HUD project files failed!"
exit 1
fi

sync

print_info "HUD application files write done!"

##################################################################
# Step 6: update uboot environment 
##################################################################
umount /dev/mmcblk0p1
EMMCupdate_v1.6 -s bootargs "console=ttySC0,115200 rw root=/dev/mmcblk0p1 rootwait loglevel=1 video=LVDS-2:1024X768-32@60" > /dev/null
EMMCupdate_v1.6 -s bootcmd "mmc dev 2:1;ext4load mmc 2:1 0x48080000 /boot/Image_test;ext4load mmc 2:1 0x48000000 /boot/r8a77990_test.dtb; booti 0x48080000 - 0x48000000" > /dev/null
EMMCupdate_v1.6 -s bootdelay "0" > /dev/null 
sync

print_info "$HUD_PROJECT project update successed!"






