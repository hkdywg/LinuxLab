/*
 *  file.c
 *  
 *  (C) 2025.03.19 <hkdywg@163.com>
 *
 *  This program is free software; you can redistribute it and/r modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 * */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>

const struct file_operations ramfs_file_operation = {
	.read_iter 		= generic_file_read_iter,
	.write_iter		= generic_file_write_iter,
	.mmap 			= generic_file_mmap,
	.fsync 			= noop_fsync,
	.splice_read 	= generic_file_splice_read,
	.splice_write 	= iter_file_splice_write,
	.llseek 		= generic_file_llseek,
};

const struct inode_operations ramfs_file_inode_operation = {
	.setattr 		= simple_setattr,
	.getattr 		= simple_getattr,
};
