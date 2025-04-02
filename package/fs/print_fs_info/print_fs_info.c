/*
 *  print_fs_info.c
 *  
 *  (C) 2025.04.01 <hkdywg@163.com>
 *
 *  This program is free software; you can redistribute it and/r modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 * */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/fdtable.h>
#include <linux/printk.h>
#include <linux/rcupdate.h>
#include <linux/kdev_t.h>
#include <linux/time.h>

void print_time_human_readable(struct timespec64 ts)
{
    struct tm result;
    time64_to_tm(ts.tv_sec, 0, &result);

    printk(KERN_INFO "Time: %04ld-%02d-%02d %02d:%02d:%02d UTC\n",
           result.tm_year + 1900, result.tm_mon + 1, result.tm_mday,
           result.tm_hour, result.tm_min, result.tm_sec);
}

static int print_vfs_super_block(struct super_block *sb)
{
    pr_info("------------super block information-------------------------\n");
    pr_info("sb->s_blocksize_bits = %d\n", sb->s_blocksize_bits);
    pr_info("sb->s_blocksize = %d\n", sb->s_blocksize);
    pr_info("sb->s_maxbytes = %x\n", sb->s_maxbytes);
    pr_info("sb->s_magic = %x\n", sb->s_magic);
    pr_info("sb->s_root = %s\n", sb->s_root->d_name.name);
    pr_info("sb->s_count = %d\n", sb->s_count);
    if(sb->s_bdev)
        pr_info("sb->s_bdev = major(%d) minor(%d)\n", MAJOR(sb->s_bdev->bd_dev), MINOR(sb->s_bdev->bd_dev));
    pr_info("sb->s_type->name = %s", sb->s_type->name);
    pr_info("bs->s_type->fs_flags = %d", sb->s_type->fs_flags);

    
    return 0;
}

static int  print_vfs_info(struct inode *inode)
{
    pr_info("------------inode information-------------------------\n");
    pr_info("inode->i_mode = %x\n", inode->i_mode);
    pr_info("inode->i_opflags = %d\n", inode->i_opflags);
    pr_info("inode->i_uid = %x\n", inode->i_uid);
    pr_info("inode->i_gid = %x\n", inode->i_gid);
    pr_info("inode->i_ino = %d\n", inode->i_ino);
    pr_info("inode->i_size = %d\n", inode->i_size);
    print_time_human_readable(inode->i_atime);
    print_time_human_readable(inode->i_mtime);
    print_time_human_readable(inode->i_ctime);
    
    return 0;
}

static int __init print_fsinfo_init(void)
{
    struct file_system_type *fs;
    struct fdtable *fdt;
    struct file **fd;
    struct file *file = filp_open("/etc/init.d/rcS", O_RDONLY, 0);
    struct inode *inode = file->f_path.dentry->d_inode;
    struct super_block *sb = file->f_path.dentry->d_sb;

    pr_info("file parent dentry name: %s\n", file->f_path.dentry->d_parent->d_name.name);
    print_vfs_info(inode);

    fs = get_fs_type("ext4");
    if(!fs) {
        pr_info("can't find filesystem !\n");
        return -1;
    }
    pr_info("------------file_system_type information-------------------------\n");
    pr_info("file_system_type->name = %s", fs->name);
    pr_info("file_system_type->fs_flags = %d", fs->fs_flags);

    rcu_read_lock();
    fdt = files_fdtable(current->files);
    fd = fdt->fd;
    pr_info("fd: %d, file name is %s\n", 0, fd[0]->f_path.dentry->d_iname);
    pr_info("fd: %d, file name is %s\n", 1, fd[1]->f_path.dentry->d_name.name);
    pr_info("fd: %d, file name is %s\n", 2, fd[2]->f_path.dentry->d_name.name);
    rcu_read_unlock();

    print_vfs_super_block(sb);
    return 0;
}

static void __exit print_fsinfo_exit(void)
{
    return;
}


module_init(print_fsinfo_init);
module_exit(print_fsinfo_exit);

MODULE_AUTHOR("yinwg hkdywg@163.com");
MODULE_DESCRIPTION("pint filesystem relation information");
MODULE_LICENSE("GPL");
