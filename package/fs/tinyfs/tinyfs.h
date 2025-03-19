/*
 *  tinyfs.h
 *  
 *  (C) 2024.08.06 <hkdywg@163.com>
 *
 *  This program is free software; you can redistribute it and/r modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 * */

#ifndef __TINY_FS_H_
#define __TINY_FS_H_

#define MAXLEN  64
#define MAX_FILES    65536
#define MAX_BLOCKSIZE   512
#define TINYFS_DEFAULT_MODE 	0755
#define TINYFS_MAGIC 			(0x19930326)

struct dir_entry {
    char filename[MAXLEN];
    uint32_t idx;
};

struct file_blk {
    uint8_t busy;
    mode_t mode;
    uint32_t idx;

    union {
        uint32_t file_size;
        uint32_t dir_child; 
    };

    char data[0];
};

struct tinyfs_mount_opts {
	umode_t mode;
};

struct tinyfs_fs_info {
	struct tinyfs_mount_opts mount_opts;
};

#endif
