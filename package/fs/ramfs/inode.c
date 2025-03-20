/*
 *  inode.c
 *  
 *  (C) 2025.03.19 <hkdywg@163.com>
 *
 *  This program is free software; you can redistribute it and/r modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/pagemap.h>
#include <linux/mm.h>

#include "internal.h"

#define TMP_RAMFS_MAGIC 	(0x142431)

static struct inode *ramfs_get_inode(struct super_block *sb,
					const struct inode *dir, umode_t mode, dev_t dev);


static int ramfs_mknod(struct inode *dir, struct dentry *dentry, 
					   umode_t mode, dev_t dev)
{
	struct inode *inode = ramfs_get_inode(dir->i_sb, dir, mode, dev);
	if(inode) {
		d_instantiate(dentry, inode);
		dget(dentry);
		dir->i_mtime = dir->i_ctime = current_time(dir);
		return 0;
	}

	return -ENOMEM;
}

static int ramfs_create(struct inode *dir, struct dentry *dentry,
						umode_t mode, bool excl)
{
	return ramfs_mknod(dir, dentry, mode | S_IFREG, 0);
}

static int ramfs_symlink(struct inode *dir, struct dentry *dentry,
						 const char *symname)
{
	struct inode *inode;
	int error = -ENOSPC;

	inode = ramfs_get_inode(dir->i_sb, dir, S_IFLNK | S_IRWXUGO, 0);
	if(inode) {
		int l = strlen(symname) + 1;
		error = page_symlink(inode, symname, l);
		if(!error) {
			d_instantiate(dentry, inode);
			dget(dentry);
			dir->i_mtime = dir->i_ctime = current_time(dir);
		} else
			iput(inode);
	}

	return error;
}

static int ramfs_mkdir(struct inode *dir, struct dentry  *dentry,
					   umode_t mode)
{
	int ret = ramfs_mknod(dir, dentry, mode | S_IFDIR, 0);
	if(!ret)
		inc_nlink(dir);

	return ret;
}

static const struct super_operations ramfs_ops = {
	.statfs = simple_statfs,
	.drop_inode = generic_delete_inode,
};

static const struct address_space_operations ramfs_aops = {
	.readpage = simple_readpage,
	.write_begin = simple_write_begin,
	.write_end =  simple_write_end,
	.set_page_dirty = noop_set_page_dirty,
};

static const struct inode_operations ramfs_dir_inode_operation = {
	.create 	= ramfs_create,
	.lookup 	= simple_lookup,
	.link 		= simple_link,
	.unlink 	= simple_unlink,
	.symlink 	= ramfs_symlink,
	.mkdir 		= ramfs_mkdir,
	.rmdir 		= simple_rmdir,
	.mknod 		= ramfs_mknod,
	.rename		= simple_rename,
};

static struct inode *ramfs_get_inode(struct super_block *sb,
					const struct inode *dir, umode_t mode, dev_t dev)
{
	struct inode *inode = new_inode(sb);

	if(inode) {
		inode->i_ino = get_next_ino();
		inode_init_owner(inode, dir, mode);
		inode->i_mapping->a_ops = &ramfs_aops;
		mapping_set_gfp_mask(inode->i_mapping, GFP_HIGHUSER);
		mapping_set_unevictable(inode->i_mapping);
		inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
		switch(mode & S_IFMT) {
		case S_IFREG:
			inode->i_op = &ramfs_file_inode_operation;
			inode->i_fop = &ramfs_file_operation;
			break;
		case S_IFDIR:
			inode->i_op = &ramfs_dir_inode_operation;
			inode->i_fop = &simple_dir_operations;
			inc_nlink(inode);
			break;
		case S_IFLNK:
			inode->i_op = &page_symlink_inode_operations;
			inode_nohighmem(inode);
			break;
		default:
			break;
		} 
	}

	return inode;
}


static int ramfs_fill_super(struct super_block *sb, void *data,int silent)
{
	struct inode *root_inode;

	sb->s_maxbytes 		 = MAX_LFS_FILESIZE;	/* max file size */
	sb->s_blocksize_bits = PAGE_SHIFT;
	sb->s_blocksize 	 = PAGE_SIZE;
	sb->s_magic 		 = TMP_RAMFS_MAGIC;
	sb->s_time_gran 	 = 1;

	root_inode = ramfs_get_inode(sb, NULL, S_IFDIR, 0);
	sb->s_root = d_make_root(root_inode);
	if(!sb->s_root)
		return -ENOMEM;

	return 0;
}

static struct dentry *ramfs_mount(struct file_system_type *fs_type,
								  int flags, const char *dev_name, void *data)
{
	return mount_nodev(fs_type, flags, data, ramfs_fill_super);
}

static void ramfs_kill_sb(struct super_block *sb)
{
	kfree(sb->s_fs_info);
	kill_litter_super(sb);
}

static struct file_system_type ramfs_type = {
	.name 		= "tmp_ramfs",
	.fs_flags 	= FS_USERNS_MOUNT,
	.mount 		= ramfs_mount,
	.kill_sb 	= ramfs_kill_sb,
};

static int __init init_tmp_ramfs(void)
{
	return register_filesystem(&ramfs_type);
}

static void __exit exit_tmp_ramfs(void)
{
	unregister_filesystem(&ramfs_type);
}

module_init(init_tmp_ramfs);
module_exit(exit_tmp_ramfs);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yinwg hkdywg@163.com");
MODULE_DESCRIPTION("tmp_ramfs filesytem");

