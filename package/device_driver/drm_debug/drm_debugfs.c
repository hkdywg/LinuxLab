/*
* drm_debugfs.c 
*   debugfs for drm system
*
*  (C) 2026.03.01 <hkdywg@163.com>
*
*  This program is free software; you can redistribute it and/r modify
*  it under the terms of the GNU General Public License version 2 as
*  published by the Free Software Foundation.
*
*/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <drm/drm_print.h>
#include <drm/drm_vblank.h>
#include <drm/drm_atomic_uapi.h>
#include <drm/drm_drv.h>
#include <drm/drm_file.h>
#include <drm/drm_gem.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_of.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_fourcc.h>
#include "drm_debug.h"

#define DUMP_BUF_PATH           "/home/root"
#define USER_BUFFER_NUM         2

static char *card_name = "card0";
module_param(card_name, charp, 0644);
MODULE_PARM_DESC(card_name, "DRM card node name, e.g. card0");

struct drm_dump_info {
    const char *plane_name;
    struct drm_framebuffer *fb;
    struct drm_rect *src;
};

struct drm_debug_display {
    struct drm_device *dev;
    struct drm_crtc *crtc;
    struct drm_plane *plane;
    struct work_struct dump_work;
    uint32_t dump_frame_count;

    struct drm_direct_show_buffer *drm_buffer[USER_BUFFER_NUM];
};

static struct drm_debug_display display_info;

static int drm_dump_plane_buffer(struct drm_dump_info *dump_info, int frame_num)
{
    struct drm_framebuffer *fb = dump_info->fb;
    struct drm_gem_object *obj = drm_gem_fb_get_obj(fb, 0);
    void *vaddr;
    char file_name[128];
    char format_name[5];
    struct file *file;
    size_t size;
    loff_t pos = 0;
    int ret = 0;

    snprintf(file_name, sizeof(file_name), "%p4cc", &dump_info->fb->format->format);
    strscpy(format_name, file_name, 5);
    
    snprintf(file_name, 128, "%s/%s_fb-%dx%d_stride-%d_%s_%d.bin",
        DUMP_BUF_PATH, dump_info->plane_name, dump_info->fb->width, dump_info->fb->height,
        dump_info->fb->pitches[0], format_name, frame_num);

    if (!obj)
        return -ENOENT;

    if (!obj->funcs || !obj->funcs->vmap) {
        DRM_ERROR("GEM object has no vmap callback\n");
        return -EOPNOTSUPP;
    }
    vaddr = obj->funcs->vmap(obj);
    if (!vaddr)
        return -ENOMEM;

    size = (size_t)fb->height * fb->pitches[0];

    file = filp_open(file_name, O_RDWR | O_CREAT, 0644);
    if (IS_ERR(file)) {
        DRM_ERROR("open %s failed: %ld\n", file_name, PTR_ERR(file));
        ret = PTR_ERR(file);
    } else {
        ret = kernel_write(file, vaddr, size, &pos);
        if (ret < 0) {
            DRM_ERROR("write %s failed: %d\n", file_name, ret);
        } else {
            DRM_INFO("dump file name is:%s\n", file_name);
        }
        filp_close(file, NULL);
    }

    if (obj->funcs && obj->funcs->vunmap)
        obj->funcs->vunmap(obj, vaddr);

    return ret;
}

static int drm_dump_buffer_show(struct seq_file *m, void *data)
{
    seq_puts(m, "rcar drm debug version: v1.0.0\n");
    seq_puts(m, "echo dump > dump Immediately dump the current frame\n");
    seq_puts(m, "echo dumpn > dump n is the number of dump frames\n");
    seq_puts(m, "dump path is /home/root\n");
    seq_puts(m, "echo pattern > display Display colorbar direct in drm system");

    return 0;
}

static int drm_dump_buffer_open(struct inode *inode, struct file *file)
{
    struct drm_debug_display *drm_debug = inode->i_private;

    return single_open(file, drm_dump_buffer_show, drm_debug);
}

static int drm_crtc_dump_plane_buffer(struct drm_crtc *crtc, int frame_num)
{
    struct drm_plane *plane;
    struct drm_plane_state *state;
    struct drm_dump_info dump_info;

    drm_atomic_crtc_for_each_plane(plane, crtc) {
        state = plane->state;
        if (!state || !state->fb)
            continue;

        dump_info.plane_name = plane->name;
        dump_info.fb = state->fb;
        dump_info.src = &state->src;

        drm_dump_plane_buffer(&dump_info, frame_num);
    }

    return 0;
}

static ssize_t drm_dump_buffer_write(struct file *file, const char __user *ubuf,
                                size_t len, loff_t *offp)
{
    struct seq_file *m = file->private_data;
    struct drm_debug_display *drm_debug = m->private;
    char buf[14] = {};
    int dump_frames = 0;
    int ret;

    if (len > sizeof(buf) - 1)
        return -EINVAL;
    if (copy_from_user(buf, ubuf, len))
        return -EFAULT;
    buf[len] = '\0';

    if (strncmp(buf, "dump", 4) == 0) {
        if (len > 5 && isdigit(buf[4])) {
            ret = kstrtoint(buf + 4, 10, &dump_frames);
            if (ret) {
                DRM_ERROR("Invalid dump frame count\n");
                return ret;
            }
            DRM_INFO("dump_frames is %d\n", dump_frames);
        }
        drm_debug->dump_frame_count = dump_frames;
        schedule_work(&drm_debug->dump_work);
    } else {
        return -EINVAL;
    }

    return len;
}

static void drm_dump_frame_work(struct work_struct *work)
{
    struct drm_debug_display *drm_debug = container_of(work, struct drm_debug_display, dump_work);
    struct drm_crtc *crtc = drm_debug->crtc;
    uint32_t frame_count;
    uint32_t i;

    frame_count = drm_debug->dump_frame_count;
    if (frame_count == 0)
        frame_count = 1;

    for (i = 0; i < frame_count; i++) {
        drm_wait_one_vblank(crtc->dev, drm_crtc_index(crtc));
        drm_modeset_lock_all(crtc->dev);
        drm_crtc_dump_plane_buffer(crtc, i);
        drm_modeset_unlock_all(crtc->dev);
    }
}

static const struct file_operations drm_dump_buffer_fops = {
    .owner = THIS_MODULE,
    .open = drm_dump_buffer_open,
    .write = drm_dump_buffer_write,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

static int drm_debug_free_buffers(struct drm_debug_display *drm_debug)
{
    int i;

    for (i = 0; i < USER_BUFFER_NUM; i++) {
        if (drm_debug->drm_buffer[i]) {
            drm_direct_show_free_buffer(drm_debug->dev, drm_debug->drm_buffer[i]);
            kfree(drm_debug->drm_buffer[i]);
            drm_debug->drm_buffer[i] = NULL;
        }
    }

    return 0;
}

static int drm_debug_alloc_buffer(struct drm_debug_display *drm_debug,
                            uint32_t width, uint32_t height, uint32_t format)
{
    int ret, i;
    struct drm_direct_show_buffer *buffer;

    drm_debug_free_buffers(drm_debug);

    for (i = 0; i < USER_BUFFER_NUM; i++) {
        buffer = kzalloc(sizeof(struct drm_direct_show_buffer), GFP_KERNEL);
        if (!buffer) {
            ret = -ENOMEM;
            goto err_free;
        }

        buffer->width = width;
        buffer->height = height;
        buffer->pixel_format = format;
        ret = drm_direct_show_alloc_buffer(drm_debug->dev, buffer);
        if (ret) {
            DRM_ERROR("Failed to alloc drm buffer: %d\n", ret);
            kfree(buffer);
            goto err_free;
        }
        drm_debug->drm_buffer[i] = buffer;
    }

    return 0;

err_free:
    drm_debug_free_buffers(drm_debug);
    return ret;
}

static int drm_debug_display_open(struct inode *inode, struct file *file)
{
    struct drm_debug_display *drm_debug = inode->i_private;

    return single_open(file, drm_dump_buffer_show, drm_debug);
}

static ssize_t drm_debug_display_write(struct file *file, const char __user *ubuf,
                                size_t len, loff_t *offp)
{
    struct seq_file *m = file->private_data;
    struct drm_debug_display *drm_debug = m->private;
    struct drm_direct_show_commit_info commit_info;
    struct drm_plane *plane;
    struct drm_plane_state *state;
    char buf[14] = {};
    int ret;

    if (len > sizeof(buf) - 1)
        return -EINVAL;
    if (copy_from_user(buf, ubuf, len))
        return -EFAULT;
    buf[len] = '\0';

    if (strncmp(buf, "pattern", 7) == 0) {
        drm_atomic_crtc_for_each_plane(plane, drm_debug->crtc) {
            state = plane->state;
            if (!state || !state->fb)
                continue;

            if (!drm_debug->drm_buffer[0]) {
                ret = drm_debug_alloc_buffer(drm_debug, state->fb->width,
                                    state->fb->height, DRM_FORMAT_BGR888);
                if (ret)
                    return ret;
            }

            drm_fill_color_bar(drm_debug->drm_buffer[0]->pixel_format, 
                               drm_debug->drm_buffer[0]->plane_vaddr, 
                               drm_debug->drm_buffer[0]->width, 
                               drm_debug->drm_buffer[0]->height, 
                               drm_debug->drm_buffer[0]->pitch[0]); 

            memset(&commit_info, 0, sizeof(commit_info));
            commit_info.crtc = drm_debug->crtc;
            commit_info.plane = plane;
            commit_info.buffer = drm_debug->drm_buffer[0];
            commit_info.src_x = 0;
            commit_info.src_y = 0;
            commit_info.src_w = drm_debug->drm_buffer[0]->width;
            commit_info.src_h = drm_debug->drm_buffer[0]->height;
            commit_info.dst_x = 0;
            commit_info.dst_y = 0;
            commit_info.dst_w = commit_info.src_w;
            commit_info.dst_h = commit_info.src_h;
            commit_info.top_zpos = true;

            ret = drm_direct_show_commit(drm_debug->dev, &commit_info);
            if (ret) {
                DRM_ERROR("drm_direct_show_commit failed: %d\n", ret);
                return ret;
            }
        }
    } else {
        return -EINVAL;
    }

    return len;
}

static const struct file_operations drm_debug_display_fops = {
    .owner = THIS_MODULE,
    .open = drm_debug_display_open,
    .write = drm_debug_display_write,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

static int drm_debug_add_file(struct drm_device *drm, struct dentry *root)
{
    struct dentry *drm_debug_root;
    struct dentry *ent;
    struct drm_crtc *crtc;
    int ret;

    crtc = drm_crtc_from_index(drm, 0);
    if (!crtc) {
        DRM_ERROR("No enabled CRTC found on %s\n", card_name);
        drm_dev_put(drm);
        return -ENODEV;
    }

    drm_debug_root = debugfs_create_dir("drm_debug", root);
    if (IS_ERR(drm_debug_root)) {
        DRM_ERROR("create drm_debug dir failed: %ld\n", PTR_ERR(drm_debug_root));
        drm_dev_put(drm);
        return PTR_ERR(drm_debug_root);
    }

    display_info.dev = drm;
    display_info.crtc = crtc;

    ent = debugfs_create_file("dump", 0444, drm_debug_root, &display_info, &drm_dump_buffer_fops);
    if (!ent) {
        DRM_ERROR("create drm_dump/dump err\n");
        debugfs_remove_recursive(drm_debug_root);
        return -ENOMEM;
    }

    ent = debugfs_create_file("display", 0444, drm_debug_root, &display_info, &drm_debug_display_fops);
    if (!ent) {
        DRM_ERROR("create drm debug display err\n");
        debugfs_remove_recursive(drm_debug_root);
        return -ENOMEM;
    }

    ret = drm_modeset_debugfs_init(drm, drm_debug_root);
    if (ret) {
        DRM_ERROR("create drm debug modeset endpoint err\n");
        debugfs_remove_recursive(drm_debug_root);
        return -ENOMEM;
    }

    return 0;
}

static struct drm_device *find_exist_drm_device(const char *card_name)
{
    struct file *filp;
    struct drm_file *file_priv;
    struct drm_device *drm = NULL;
    char path[32];
    int err;

    if (!card_name || !*card_name)
        return ERR_PTR(-EINVAL);

    snprintf(path, sizeof(path), "/dev/dri/%s", card_name);
    filp = filp_open(path, O_RDONLY, 0);
    if (IS_ERR(filp)) {
        err = PTR_ERR(filp);
        if (err == -ENOENT || err == -ENODEV)
            return ERR_PTR(-EPROBE_DEFER);
        return ERR_PTR(err);
    }

    file_priv = filp->private_data;
    if (!file_priv || !file_priv->minor || !file_priv->minor->dev) {
        fput(filp);
        return ERR_PTR(-ENODEV);
    }

    drm = file_priv->minor->dev;
    drm_dev_get(drm);
    fput(filp);

    return drm;
}

static int drm_debug_probe(struct platform_device *pdev)
{
    struct drm_device *drm;
    int ret;

    drm = find_exist_drm_device(card_name);
    if (IS_ERR(drm))
        return PTR_ERR(drm);

    if (!drm->primary || !drm->primary->debugfs_root) {
        DRM_ERROR("Debugfs root not available for %s\n", card_name);
        drm_dev_put(drm);
        return -EPROBE_DEFER;
    }

    INIT_WORK(&display_info.dump_work, drm_dump_frame_work);

    ret = drm_debug_add_file(drm, drm->primary->debugfs_root);
    if (ret) {
        DRM_ERROR("drm_debug_add_file failed: %d\n", ret);
        flush_work(&display_info.dump_work);
        drm_dev_put(drm);
        return ret;
    }

    platform_set_drvdata(pdev, drm);
    return 0;
}

static int drm_debug_remove(struct platform_device *pdev)
{
    struct drm_device *drm = platform_get_drvdata(pdev);

    cancel_work_sync(&display_info.dump_work);
    drm_debug_free_buffers(&display_info);

    if (drm)
        drm_dev_put(drm);

    return 0;
}

static struct platform_driver drm_debug_driver = {
    .probe = drm_debug_probe,
    .remove = drm_debug_remove,
    .driver = {
        .name = "drm-debug",
    },
};

static struct platform_device *drm_debug_pdev;

static int __init drm_debug_init(void)
{
    int ret;

    ret = platform_driver_register(&drm_debug_driver);
    if (ret)
        return ret;

    drm_debug_pdev = platform_device_register_simple("drm-debug", -1, NULL, 0);
    if (IS_ERR(drm_debug_pdev)) {
        ret = PTR_ERR(drm_debug_pdev);
        platform_driver_unregister(&drm_debug_driver);
        return ret;
    }

    return 0;
}

static void __exit drm_debug_exit(void)
{
    if (drm_debug_pdev && !IS_ERR(drm_debug_pdev))
        platform_device_unregister(drm_debug_pdev);
    platform_driver_unregister(&drm_debug_driver);
}

module_init(drm_debug_init);
module_exit(drm_debug_exit);

MODULE_AUTHOR("yinwg");
MODULE_DESCRIPTION("drm debug dump driver");
MODULE_LICENSE("GPL");
