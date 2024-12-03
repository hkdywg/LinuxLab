/*
 *  tinyfs.c
 *  
 *  (C) 2024.08.06 <hkdywg@163.com>
 *
 *  This program is free software; you can redistribute it and/r modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 * */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/time.h>
#include <linux/uaccess.h>

#include "tinyfs.h"

struct file_blk block[MAX_FILES + 1];
int curr_count = 0;
static struct inode_operations tinyfs_inode_ops;
const struct file_operations tinyfs_dir_operations;
const struct file_operations tinyfs_file_operations;
static int tinyfs_readdir(struct file *file, struct dir_context *ctx);

static int get_block(void)
{
    int i;

    for(i = 2; i < MAX_FILES; i++){
        if(!block[i].busy) {
            block[i].busy = 1;
            return i;
        }
    }

    return -1;
}


static int tinyfs_do_create(struct inode *dir, struct dentry *dentry, umode_t mode)
{
    struct inode *inode;
    struct super_block *sb;
    struct dir_entry *entry;
    struct file_blk *blk, *pblk;
    int idx;

    sb = dir->i_sb;
    if(curr_count >= MAX_FILES)
        return -ENOSPC;

    if(!S_ISDIR(mode) && !(S_ISREG(mode)))
        return -EINVAL;

    if(!(inode = new_inode(sb)))
        return -ENOMEM;

    inode->i_sb = sb;
    inode->i_op = &tinyfs_inode_ops;
    inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);

    idx = get_block();      // get next idle block for new file

    blk = &block[idx];
    inode->i_ino = idx;
    blk->mode = mode;
    curr_count++;

    if(S_ISDIR(mode)) {
        blk->dir_child = 0;
        inode->i_fop = &tinyfs_dir_operations;
    } else {
        blk->file_size = 0;
        inode->i_fop = &tinyfs_file_operations;
    }

    inode->i_private = blk;
    pblk = (struct file_blk *)dir->i_private;

    entry = (struct dir_entry *)&pblk->data[0];
    entry += pblk->dir_child;
    pblk->dir_child ++;
    
    entry->idx = idx;
    strcpy(entry->filename, dentry->d_name.name);

    inode_init_owner(inode, dir, mode);
    d_add(dentry, inode);

    return 0;
}

static int tinyfs_create(struct inode *dir, struct dentry *dentry, umode_t mode, bool excl)
{
    return tinyfs_do_create(dir, dentry, mode);
}

static struct inode *tinyfs_iget(struct super_block *sb, int idx)
{
    struct inode *inode;
    struct file_blk *blk;

    inode = new_inode(sb);
    inode->i_ino = idx;
    inode->i_sb = sb;
    inode->i_op = &tinyfs_inode_ops;

    blk = &block[idx];
    if(S_ISDIR(blk->mode))
        inode->i_fop = &tinyfs_dir_operations;
    else if(S_ISREG(blk->mode))
        inode->i_fop = &tinyfs_file_operations;

    inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
    inode->i_private = blk;

    return inode;
}

struct dentry *tinyfs_lookup(struct inode *parent_inode, struct dentry *child_dentry, unsigned int flags)
{
    struct super_block *sb = parent_inode->i_sb;
    struct file_blk *blk;
    struct dir_entry *entry;
    int i;

    blk = (struct file_blk *)parent_inode->i_private;
    entry = (struct dir_entry *)&blk->data[0];
    for(i = 0; i < blk->dir_child; i++) {
        if(!strcmp(entry[i].filename, child_dentry->d_name.name)) {
            struct inode *inode = tinyfs_iget(sb, entry[i].idx);
            struct file_blk *inner = (struct file_blk *)inode->i_private;
            inode_init_owner(inode, parent_inode, inner->mode);
            d_add(child_dentry, inode);

            return NULL;
        }
    }

    return NULL;
}

int tinyfs_rmdir(struct inode *dir, struct dentry *dentry)
{
    struct inode *inode = dentry->d_inode;
    struct file_blk *blk = (struct file_blk *)inode->i_private;

    blk->busy = 0;

    return simple_rmdir(dir, dentry);
}

int tinyfs_unlink(struct inode *dir, struct dentry *dentry)
{
    int i;
    struct inode *inode = dentry->d_inode;
    struct file_blk *blk = (struct file_blk *)inode->i_private;
    struct file_blk *pblk = (struct file_blk *)dir->i_private;
    struct dir_entry *entry;

    entry = (struct dir_entry *)&pblk->data[0];
    for(i = 0; i < pblk->dir_child; i++) {
        if(!strcmp(entry[i].filename, dentry->d_name.name)) {
            int j;
            for(j = i; j < pblk->dir_child - 1; j++) 
                memcpy(&entry[j], &entry[j+1], sizeof(struct dir_entry));
            pblk->dir_child --;
            break;
        }
    }
    
    blk->busy = 0;
    return simple_unlink(dir, dentry);
}

static int tinyfs_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode)
{
    return tinyfs_do_create(dir, dentry, S_IFDIR | mode);
}


ssize_t tinyfs_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos)
{
    struct file_blk *blk;
    char *buffer;

    blk = (struct file_blk *)filp->f_path.dentry->d_inode->i_private;
    if(*ppos >= blk->file_size)
        return 0;
    
    buffer = (char *)&blk->data[0];
    len = min((size_t)blk->file_size, len);

    if(copy_to_user(buf, buffer, len))
        return -EFAULT;

    *ppos += len;

    return len;
}


ssize_t tinyfs_write(struct file *filp, const char __user *buf, size_t len, loff_t *ppos)
{
    struct file_blk *blk;
    char *buffer;

    blk = filp->f_path.dentry->d_inode->i_private;

    buffer = (char *)&blk->data[0];
    buffer += *ppos;

    if(copy_from_user(buffer, buf, len))
        return -EFAULT;

    *ppos += len;
    blk->file_size = *ppos;

    return len;
}


static int tinyfs_readdir(struct file *file, struct dir_context *ctx)
{
    loff_t pos;
    struct file_blk *blk;
    struct dir_entry *entry;
    int i;

    pos = file->f_pos;
    if(pos)
        return 0;

    blk = (struct file_blk *)file->f_path.dentry->d_inode->i_private;
    if(!S_ISDIR(blk->mode))
        return -ENOTDIR;

    entry = (struct dir_entry *)&blk->data[0];
    for(i = 0; i < blk->dir_child; i++) {
		dir_emit(ctx, entry[i].filename, MAXLEN, entry[i].idx, DT_UNKNOWN);
        file->f_pos += sizeof(struct dir_entry);
        pos += sizeof(struct dir_entry);
		ctx->pos = pos;
    }

    return 0;
}

ssize_t tinyfs_generic_read_dir(struct file *filp, char __user *buf, size_t siz, loff_t *ppos)
{
	return -EISDIR;
}

static struct inode_operations tinyfs_inode_ops = {
    .create = tinyfs_create,
    .lookup = tinyfs_lookup,
    .mkdir = tinyfs_mkdir,
    .rmdir = tinyfs_rmdir,
    .unlink = tinyfs_unlink,
};

const struct file_operations tinyfs_file_operations = {
    .read = tinyfs_read,
    .write = tinyfs_write,
};

const struct file_operations tinyfs_dir_operations = {
    .owner = THIS_MODULE,
    .read = tinyfs_generic_read_dir,
    //.readdir = tinyfs_readdir,
    .iterate_shared = tinyfs_readdir,
};


int tinyfs_fill_super(struct super_block *sb, void *data, int silent)
{
    struct inode *root_inode;
    int mode = S_IFDIR;

    root_inode = new_inode(sb);
    root_inode->i_ino = 1;
    inode_init_owner(root_inode, NULL, mode);
    root_inode->i_sb = sb;
    root_inode->i_op = &tinyfs_inode_ops;
    root_inode->i_fop = &tinyfs_dir_operations;
    root_inode->i_atime = root_inode->i_mtime = root_inode->i_ctime = current_time(root_inode);

    block[1].mode = mode;
    block[1].dir_child = 0;
    block[1].idx = 1;
    block[1].busy = 1;
    root_inode->i_private = &block[1];

    sb->s_root = d_make_root(root_inode);
    curr_count++;

    return 0;
}

static struct dentry *tinyfs_mount(struct file_system_type *fs_type, int flags, const char *dev_name, void *data)
{
    return mount_nodev(fs_type, flags, data, tinyfs_fill_super);
}

static void tinyfs_kill_superblock(struct super_block *sb)
{
    kill_anon_super(sb);
}


struct file_system_type tinyfs_fs_type = {
    .owner = THIS_MODULE,
    .name = "tinyfs",
    .mount = tinyfs_mount,
    .kill_sb = tinyfs_kill_superblock,
};

static int __init tinyfs_init(void)
{
    int ret;

    memset(block, 0, sizeof(block));
    ret = register_filesystem(&tinyfs_fs_type);
    if(ret)
        printk("register tinyfs failed!\n");

    return ret;
}

static void __exit tinyfs_exit(void)
{
    unregister_filesystem(&tinyfs_fs_type);
}


module_init(tinyfs_init);
module_exit(tinyfs_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("yinwg <hkdywg@163.com>");
MODULE_DESCRIPTION("kernel tiny file system");
