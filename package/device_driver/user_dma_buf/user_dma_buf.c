/*
 *  user_dma_buf.c
 *  
 *  (C) 2026.03.25 <hkdywg@163.com>
 *
 *  This program is free software; you can redistribute it and/r modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 * */
#include <linux/cdev.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/fs.h>
#include <linux/idr.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/sysctl.h>
#include <linux/types.h>
#include <linux/pagemap.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>

#include <linux/of_reserved_mem.h>
#include <linux/dma-buf.h>
#include <linux/dma-direct.h>
#include <linux/dma-map-ops.h>
#include <linux/iommu.h>

#define DRIVER_NAME         "u-dma-buf"
#define DEVICE_NAME_FORMAT  "udmabuf%d"
#define DEVICE_MAX_NUM      256

struct udmabuf_object {
    struct device *sys_dev;
    struct device *dma_dev;
    struct cdev cdev;
    dev_t device_num;
    struct mutex sem;
    bool is_open;
    size_t size;
    size_t alloc_size;
    void *virt_addr;
    dma_addr_t phys_addr;
    int sync_mode;
    u64 sync_offset;
    size_t sync_size;
    int sync_direction;
    bool sync_owner;
    u64 sync_for_cpu;
    u64 sync_for_device;
    int quirk_mmap_mode;
    pgoff_t pagecount;
    struct page **pages;
    struct list_head export_dma_buf_list;
    struct mutex export_dma_buf_list_sem;
    bool of_reserved_mem;
    int debug_vma;
    bool debug_export;
};


static struct class *udmabuf_sys_class = NULL;
static bool udmabuf_platform_driver_registered = false;

static DEFINE_IDA(udmabuf_device_ida);
static dev_t udmabuf_device_number = 0;
static struct list_head udmabuf_device_list;
static struct mutex udmabuf_device_list_sem;
#define ida_simple_get(ida, start, end, gfp) ida_alloc_range(ida, start, (end) -1 , gfp)
#define ida_simple_remove(ida, id)           ida_free(ida, id)

#define DEF_ATTR_SHOW(__attr_name, __format, __value) \
static ssize_t udmabuf_show_ ## __attr_name(struct device *dev, struct device_attribute *attr, char *buf) \
{                                                       \
    ssize_t status;                                     \
    struct udmabuf_object *obj = dev_get_drvdata(dev);  \
    if (mutex_lock_interruptible(&obj->sem) != 0)       \
        return -ERESTARTSYS;                            \
    status = sprintf(buf, __format, (__value));         \
    mutex_unlock(&obj->sem);                            \
    return status;                                      \
}

#define DEF_ATTR_SET(__attr_name, __min, __max, __pre_action, __post_action) \
static ssize_t udmabuf_set_ ## __attr_name(struct device *dev, struct device_attribute *attr, const char *buf, size_t size) \
{                                                       \
    ssize_t status;                                     \
    u64 value;                                          \
    struct udmabuf_object *obj = dev_get_drvdata(dev);  \
    if (mutex_lock_interruptible(&obj->sem) != 0 )      \
        return -ERESTARTSYS;                            \
    if ((status = kstrtoull(buf, 0, &value)) != 0)      \
        goto failed;                                    \
    if ((value < __min) || (value > __max)) {           \
        status = -EINVAL;                               \
        goto failed;                                    \
    }                                                   \
    if ((status = __pre_action(obj)) != 0)              \
        goto failed;                                    \
    obj->__attr_name = value;                           \
    if ((status = __post_action(obj)) != 0)             \
        goto failed;                                    \
    status = size;                                      \
failed:                                                 \
    mutex_unlock(&obj->sem);                            \
    return status;                                      \
}                                                       

static inline int NO_ACTION(struct udmabuf_object *obj) 
{
    return 0;
}

DEF_ATTR_SHOW(size,           , "%zu\n"     ,    obj->size,                    );
DEF_ATTR_SHOW(phys_addr,      , "%pad\n"    ,    obj->phys_addr,               );
DEF_ATTR_SHOW(sync_mode       , "%d\n"      ,    obj->sync_mode                );
DEF_ATTR_SHOW(sync_offset     , "0x%llx\n"  ,    obj->sync_offset              );
DEF_ATTR_SHOW(sync_size       , "%zu\n"     ,    obj->sync_size                );
DEF_ATTR_SHOW(sync_direction  , "%d\n"      ,    obj->sync_direction           );
DEF_ATTR_SHOW(sync_owner      , "%d\n"      ,    obj->sync_owner               );
DEF_ATTR_SHOW(sync_for_cpu    , "%llu\n"    ,    obj->sync_for_cpu             );
DEF_ATTR_SHOW(sync_for_device , "%llu\n"    ,    obj->sync_for_device          );
DEF_ATTR_SHOW(quirk_mmap_mode , "%d\n"      ,    obj->quirk_mmap_mode          );
DEF_ATTR_SHOW(dma_coherent    , "%d\n"      ,    IS_DMA_COHERENT(obj->dma_dev) );
DEF_ATTR_SHOW(debug_vma       , "%d\n"      ,    obj->debug_vma                );
DEF_ATTR_SHOW(debug_export    , "%d\n"      ,    obj->debug_export             );

DEF_ATTR_SET(sync_mode       , 0,   7        ,   NO_ACTION,  NO_ACTION               );
DEF_ATTR_SET(sync_offset     , 0,   U64_MAX  ,   NO_ACTION,  NO_ACTION               );
DEF_ATTR_SET(sync_size       , 0,   SIZE_MAX ,   NO_ACTION,  NO_ACTION               );
DEF_ATTR_SET(sync_direction  , 0,   2        ,   NO_ACTION,  NO_ACTION               );
DEF_ATTR_SET(sync_for_cpu    , 0,   U64_MAX  ,   NO_ACTION,  udmabuf_sync_for_cpu    );
DEF_ATTR_SET(sync_for_device , 0,   U64_MAX  ,   NO_ACTION,  udmabuf_sync_for_device );
DEF_ATTR_SET(debug_vma       , 0,   3        ,   NO_ACTION,  NO_ACTION               );
DEF_ATTR_SET(debug_export    , 0,   1        ,   NO_ACTION,  NO_ACTION               );

                                                        

static struct device_attribute udmabuf_device_attrs[] = {
    __ATTR(size            , 0444, udmabuf_show_size            , NULL                        ),
    __ATTR(phys_addr       , 0444, udmabuf_show_phys_addr       , NULL                        ),
    __ATTR(sync_mode       , 0664, udmabuf_show_sync_mode       , udmabuf_set_sync_mode       ),
    __ATTR(sync_offset     , 0664, udmabuf_show_sync_offset     , udmabuf_set_sync_offset     ),
    __ATTR(sync_size       , 0664, udmabuf_show_sync_size       , udmabuf_set_sync_size       ),
    __ATTR(sync_direction  , 0664, udmabuf_show_sync_direction  , udmabuf_set_sync_direction  ),
    __ATTR(sync_owner      , 0444, udmabuf_show_sync_owner      , NULL                        ),
    __ATTR(sync_for_cpu    , 0664, udmabuf_show_sync_for_cpu    , udmabuf_set_sync_for_cpu    ),
    __ATTR(sync_for_device , 0664, udmabuf_show_sync_for_device , udmabuf_set_sync_for_device ),
    __ATTR(quirk_mmap_mode , 0444, udmabuf_show_quirk_mmap_mode , NULL                        ),
    __ATTR(dma_coherent    , 0444, udmabuf_show_dma_coherent    , NULL                        ),
    __ATTR(debug_vma       , 0664, udmabuf_show_debug_vma       , udmabuf_set_debug_vma       ),
    __ATTR(debug_export    , 0664, udmabuf_show_debug_export    , udmabuf_set_debug_export    ),
    __ATTR_NULL,
};

#define udmabuf_device_attrs_size (sizeof(udmabuf_device_attrs/sizeof(udmabuf_device_attrs[0])))

static struct attribute *udmabuf_attrs[udmabuf_device_attrs_size] = {
    NULL
};

static struct attribute_group udmabuf_attr_group = {
    .attrs = udmabuf_attrs
};

static const struct attribute_group *udmabuf_attr_groups[] = {
    &udmabuf_attr_group,
    NULL
};

static inline void udmabuf_sys_class_set_attribute(void)
{
    int i;
    for (i = 0; i < udmabuf_device_attrs_size - 1; i++) {
        udmabuf_attrs[i] = &(udmabuf_device_attrs[i].attr);
    }
    udmabuf_attrs[i] = NULL;
    udmabuf_sys_class->dev_groups = udmabuf_attr_groups;
}

#define DEFINE_UDMABUF_STATIC_DEVICE_PARAM(__num)                           \
    static ulong udmabuf ## __num = 0;                                      \
    module_param(udmabuf ## __num, ulong, S_IRUGO);                         \
    MODULE_PARM_DESC(udmabuf ## __num, DRIVER_NAME #__num " buffer size");  \
    static char *udmabuf ## __num ## _bind = NULL;                          \
    module_param(udmabuf ## __num ## _bind, charp, S_IRUGO);                \
    MODULE_PARM_DESC(udmabuf ## __num ## _bind, DRIVER_NAME #__num,         \
            " bind device name. exp pci/0000:00:20:0");

#define CALL_UDMABUF_STATIC_DEVICE_RESERVE_MINOR_NUMBER(__num)              \
    if (udmabuf ## __num != 0)  {                                           \
        ida_simple_get(&udmabuf_device_ida, __num, __num + 1, GFP_KERNEL);  \
    }

#define CALL_UDMABUF_STATIC_DEVICE_CREATE(__num)        \
    if (udmabuf ## __num != 0) {                        \
        int ret;                                        \
        udmabuf_static_device_param param;              \
        ida_simple_remove(&udmabuf_device_ida, __num);  \
        param.name  = NULL;                             \
        param.id    = __num;                            \
        param.size  = udmabuf ## __num;                 \
        param.bind_id = udmabuf ## __num ## _bind;      \
        ret = udmabuf_static_device_create(&param);     \
        if (ret)                                        \
            status = ret;                               \
    }                                                   

#define DEFINE_UDMABUF_OPTION(name,type,lo,hi)              \
static inline type udmabuf_get_option_ ## name(u64 option)  \
{                                                           \
    const u64 mask = ((1UL << ((hi) - (lo) + 1)) - 1);      \
    return (type)((option >> (lo)) & mask);                 \
}

DEFINE_UDMABUF_OPTION(dma_mask_size   , u64,  0, 7 )
DEFINE_UDMABUF_OPTION(quirk_mmap_mode , int, 10, 12)

static inline bool udmabuf_check_quirk_mmap_mode(int value)
{
    bool is_valid = false;
    is_valid |= (value == QUIRK_MMAP_MODE_ALWAYS_OFF);
    is_valid |= (value == QUIRK_MMAP_MODE_ALWAYS_ON);
    is_valid |= (value == QUIRK_MMAP_MODE_AUTO);
    is_valid |= (value == QUIRK_MMAP_MODE_PAGE);
    return is_valid;
}

static int udmabuf_get_quirk_mmap_property(struct device *dev, int *value, bool lock)
{
    u64 option;
    int status = device_property_read_u64(dev, "option", &option);
    if (status == 0) {
        int quirk_mmap_mode = udmabuf_get_option_quirk_mmap_mode(option);
        if (udmabuf_check_quirk_mmap_mode(quirk_mmap_mode) == true)
            *value = quirk_mmap_mode;
        else
            status = -EINVAL;
    }
    return status;
}

static inline int udmabuf_set_quirk_mmap_mode(struct udmabuf_object *obj, int value)
{
     if (!obj)
         return -ENODEV;

     if (udmabuf_check_quirk_mmap_mode(value) == false)
         return -EINVAL;

     obj->quirk_mmap_mode = value;

     return 0;
}
                                                        

static struct udmabuf_object *udmabuf_object_create(const char *name, struct device *parent, int minor)
{
    struct udmabuf_object *obj = NULL;
    unsigned int done = 0;
    const unsigned int DONE_ALLOC_MINOR   = (1 << 0);
    const unsigned int DONE_CHRDEV_ADD    = (1 << 1);
    const unsigned int DONE_DEVICE_CREATE = (1 << 3);
    const unsigned int DONE_SET_DMA_DEV   = (1 << 4);
    int ret;

    if ((0 <= minor) && (minor < DEVICE_MAX_NUM)) {
        if (ida_simple_get(&udmabuf_device_ida, minor, minor+1, GFP_KERNEL) < 0) {
            pr_err(DRIVER_NAME ": couldn't allocat minor number(=%d).\n", minor);
            goto failed;
        } 
    } else if (minor < 0) {
        if ((minor = ida_simple_get(&udmabuf_device_ida, 0, DEVICE_MAX_NUM, GFP_KERNEL)) < 0) {
            pr_err(DRIVER_NAME ": couldn't allocat new minor number, return = %d.\n", minor);
            goto failed;
        }
    } else {
        pr_err(DRIVER_NAME ": invalid minor number(=%d), valid range is 0 to %d\n", minor, DEVICE_MAX_NUM-1);
        goto failed;
    }
    done |= DONE_ALLOC_MINOR;

    obj = kzalloc(sizeof(*obj), GFP_KERNEL);
    if (IS_ERR_OR_NULL(obj)) {
        ret = PTR_ERR(obj);
        obj = NULL;
        pr_err(DRIVER_NAME ": kzalloc failed, return %d\n", ret);
        goto failed;
    }
    obj->device_number = MKDEV(MAJOR(udmabuf_device_num), minor);

    if (name == NULL)
        obj->sys_dev = device_create(udmabuf_sys_class, parent, obj->device_number,
                            (void *)obj, DEVICE_NAME_FORMAT, MINOR(obj->device_number));
    else
        obj->sys_dev = device_create(udmabuf_sys_class, parent, obj->device_number,
                            (void *)obj, "%s", name);
    if (IS_ERR_OR_NULL(obj->sys_dev)) {
        obj->sys_dev = NULL;
        pr_err(DRIVER_NAME ": device_create failed, return = %d\n", PTR_ERR(obj->sys_dev));
        goto failed;
    }
    done |= DONE_DEVICE_CREATE;

    cdev_init(&obj->cdev, &udmabuf_device_file_ops);
    obj->cdev.owner = THIS_MODULE;
    if (cdev_add(&obj->cdev, obj->device_number, 1) != 0) {
        dev_err(obj->sys_dev, "cdev_add failed.\n");
        goto failed;
    }
    done |= DONE_CHRDEV_ADD;

    if (parent != NULL)
        obj->dma_dev = get_device(parent);
    else
        obj->dma_dev = get_device(obj->sys_dev);

    if (obj->dma_dev->dma_mask == NULL)
        obj->dma_dev->dma_mask = &obj->dma_dev->coherent_dma_mask;

    if (*obj->dma_dev->dma_mask == 0) {
        if (dma_set_mask_and_coherent(obj->dma_dev, DMA_BIT_MASK(dma_mask_bit)) != 0) {
            dev_warn(obj->sys_dev, "dma_set_mask_and_coherent(DMA_BIT_MASK(%d)) failed.\n", dma_mask_bit);
            *obj->dma_dev->dma_mask = DMA_BIT_MASK(dma_mask_bit);
            obj->dma_dev->coherent_dma_mask = DMA_BIT_MASK(dma_mask_bit);
        }
    }
    done |= DONE_SET_DMA_DEV;

    obj->size = 0;
    obj->alloc_size = 0;
    obj->sync_mode = SYNC_MODE_NONCACHED;
    obj->sync_offset = 0;
    obj->sync_size = 0;
    obj->sync_direction = 0;
    obj->sync_owner = 0;
    obj->sync_for_cpu = 0;
    obj->sync_for_device = 0;
    obj->of_reserved_mem = 0;
    obj->quirk_mmap_mode = quirk_mmap_mode;
    obj->pagecount = 0;
    obj->pages = NULL;
    INIT_LIST_HEAD(&obj->export_dma_buf_list);
    mutex_init(&obj->export_dma_buf_list_sem);
    obj->debug_vma = 0;
    obj->debug_export = 0;
    mutex_init(&obj->sem);

    return obj;

failed:
    if (done & DONE_SET_DMA_DEV) put_device(obj->dma_dev);
    if (done & DONE_CHRDEV_ADD) cdev_del(&obj->cdev);
    if (done & DONE_DEVICE_CREATE) device_destroy(udmabuf_sys_class, obj->device_number);
    if (done & DONE_ALLOC_MINOR) ida_simple_remove(&udmabuf_device_ida, minor);
    if (obj != NULL) kfree(obj);
    return NULL;

}

static void udmabuf_static_device_reserve_minor_number_all(void)
{
    CALL_UDMABUF_STATIC_DEVICE_RESERVE_MINOR_NUMBER(0);
    CALL_UDMABUF_STATIC_DEVICE_RESERVE_MINOR_NUMBER(1);
    CALL_UDMABUF_STATIC_DEVICE_RESERVE_MINOR_NUMBER(2);
    CALL_UDMABUF_STATIC_DEVICE_RESERVE_MINOR_NUMBER(3);
    CALL_UDMABUF_STATIC_DEVICE_RESERVE_MINOR_NUMBER(4);
    CALL_UDMABUF_STATIC_DEVICE_RESERVE_MINOR_NUMBER(5);
    CALL_UDMABUF_STATIC_DEVICE_RESERVE_MINOR_NUMBER(6);
    CALL_UDMABUF_STATIC_DEVICE_RESERVE_MINOR_NUMBER(7);
}

static int udmabuf_static_device_create_all(void)
{
    int status = 0;
    CALL_UDMABUF_STATIC_DEVICE_CREATE(0);
    CALL_UDMABUF_STATIC_DEVICE_CREATE(1);
    CALL_UDMABUF_STATIC_DEVICE_CREATE(2);
    CALL_UDMABUF_STATIC_DEVICE_CREATE(3);
    CALL_UDMABUF_STATIC_DEVICE_CREATE(4);
    CALL_UDMABUF_STATIC_DEVICE_CREATE(5);
    CALL_UDMABUF_STATIC_DEVICE_CREATE(6);
    CALL_UDMABUF_STATIC_DEVICE_CREATE(7);
    return status;
}

static int udmabuf_object_setup(struct udmabuf_object *obj)
{
    if (!obj)
        return -ENODEV;

    obj->alloc_size = ((obj->size + (((size_t)1 << PAGE_SHIFT) - 1)) >> PAGE_SHIFT) << PAGE_SHIFT;

    obj->virt_addr = dma_alloc_coherent(obj->dma_dev, obj->alloc_size, &obj->phys_addr, GFP_KERNEL);
    if (IS_ERR_OR_NULL(obj->virt_addr)) {
        dev_err(obj->sys_dev, "dma_alloc_coherent(size = %zu) failed. return(%d\n)", obj->alloc_size, PTR_ERR(obj->virt_addr));
        obj->virt_addr = NULL;
        return (PTR_ERR(obj->virt_addr) == 0) ? -ENOMEM : PTR_ERR(obj->virt_addr); 
    }
    
    if (obj->quirk_mmap_mode == QUIRK_MMAP_MODE_PAGE) {
        pgoff_t pg;
        phys_addr_t phys_paddr = dma_to_phys(obj->dma_dev, obj->phys_addr);
        unsigned long page_frame_num = phys_paddr >> PAGE_SHIFT;
        struct page *phys_pages;

        if (!pfn_valid(page_frame_num)) {
            dev_warn(obj->sys_dev, "get page(phys_addr=%pad) failed.\n", &obj->phys_addr);
            goto quirk_mmap_page_done;
        }

        phys_pages = pfn_to_page(page_frame_num);
        obj->pagecount = obj->alloc_size >> PAGE_SHIFT;
        obj->pages = kmalloc_array(obj->pagecount, sizeof(struct *page), GFP_KERNEL);
        if (IS_ERR_OR_NULL(obj->pages)) {
            dev_warn(obj->sys_dev, "allocate pages(pagecount=%lu) failed. return(%d\n)", 
                     (unsigned long)obj->pagecount, PTR_ERR(obj->pages));
            obj->pagecount = 0;
            obj->pages = NULL;
            goto quirk_mmap_page_done;
        }
        for (pg = 0; pg < obj->pagecount; pg++) {
            obj->pages[pg] = &phys_pages[pg];
            page_kasan_tag_reset(obj->pages[pg]);
        }
        quirk_mmap_page_done:
        ;
    }

    return 0;
}

static int udmabuf_object_destroy(struct udmabuf_object *obj)
{
    if (!obj)
        return -ENODEV;
    mutex_lock(&obj->export_dma_buf_list_sem);
    if (list_empty(&obj->export_dma_buf_list) == false) {
        dev_err(obj->sys_dev, "exported dma-buf is currently busy.\n");
        mutex_unlock(&obj->export_dma_buf_list_sem);
        return -EBUSY;
    }
    mutex_unlock(&obj->export_dma_buf_list_sem);

    if (obj->pages != NULL) {
        kfree(obj->pages);
        obj->pages = NULL;
        obj->pagecount = 0;
    }

    if (obj->virt_addr != NULL) {
        dma_free_coherent(obj->dma_dev, obj->alloc_size, obj->virt_addr, obj->phys_addr);
        obj->virt_addr = NULL;
    }
    put_device(obj->dma_dev);
    cdev_del(&obj->cdev);
    device_destroy(udmabuf_sys_class, obj->device_number);
    ida_simple_remove(&udmabuf_device_ida, MINOR(obj->device_number));
    kfree(obj);

    return 0;
}

static int udmabuf_platform_device_remove(struct device *dev, struct udmabuf_object *obj)
{
    int ret;

    if (obj != NULL) {
        bool of_reserved_mem = obj->of_reserved_mem;
        ret = udmabuf_object_destroy(obj);
        if (ret != 0) {
            dev_set_drvdata(dev, NULL);
            if (of_reserved_mem)
                of_reserved_mem_release(dev);
        }
    } else {
        ret = -ENODEV;
    }

    return ret;
}

static int udmabuf_platform_device_probe(struct device *dev)
{
    int ret, minor_number;
    u32 u32_vaule;
    u64 u64_value;
    size_t size;
    struct udmabuf_object *obj = NULL;
    const char *device_name;
    int quirk_mmap_mode;

    if (device_property_read_u64(dev, "size", &u64_value) == 0) {
        size = u64_value;
    } else if ((ret = of_property_read_ulong(dev->of_node, "size", &u64_value)) == 0) {
        size = u64_value;
    } else {
        dev_err(dev, "invalid property size. status = %d\n", ret);
        ret = -ENODEV;
        goto failed;
    }

    if (size <= 0) {
        dev_err(dev, "invalid size, size = %d\n", size);
        ret = -ENODEV;
        goto failed;
    }
    
    if (device_property_read_u32(dev, "minor-number", &u32_value) == 0) {
        minor_number = u32_value;
    } else if ((ret = of_property_read_ulong(dev->of_node, "minor-number", &u64_value)) == 0) {
        minor_number = u32_value;
    } else {
        minor_number = -1;
    }

    if (device_property_read_string(dev, "device-name", &device_name) != 0)
        device_name = of_get_property(dev->of_node, "device-name", NULL);
    if (IS_ERR_OR_NULL(device_name)) {
        if (minor_number < 0)
            device_name = dev_name(dev);
        else 
            device_name = NULL;
    }

    obj = udmabuf_object_create(device_name, dev, minor_number);
    if (IS_ERR_OR_NULL(obj)) {
        ret = PTR_ERR(obj);
        dev_err(dev, "object create failed. return = %d\n", ret);
        obj = NULL;
        ret = (ret == 0) ? -EINVAL : ret;
        goto failed;
    }

    mutex_lock(&obj->sem);
    dev_set_drvdata(dev, obj);
    obj->size = size;
    if (of_property_read_u32(dev->of_node, "dma-mask", &u32_value) == 0) {
        if ((u32_value > 64) || (u32_value < 12)) {
            dev_err(dev, "invalid dma-mask property value = %d\n", u32_value);
            goto failed_with_unlock;
        }
        ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(u32_value));
        if (ret != 0) {
            dev_info(dev, "dma_set_mask_and_coherent(dev, DMA_BIT_MASK(%d)) failed. return = %d\n", u32_value, ret);
            ret = 0;
            *dev->dma_mask = DMA_BIT_MASK(u32_mask);
            dev->coherent_dma_mask = DMA_BIT_MASK(u32_mask);
        }
    }

    if (dev->of_node != NULL) {
        ret = of_reserved_mem_device_init(dev);
        if (ret == 0) {
            obj->of_reserved_mem = 1;
        } else if (ret != -ENODEV) {
            dev_err(dev, "of_reserved_mem_device_init failed\n");
            goto failed_with_unlock;
        }
    }

    if (obj->of_reserved_mem == 0) {
        ret = of_dma_configure(dev, dev->of_node, true);
        if (ret != 0) {
            dev_err(dev, "of_dma_configure failed\n");
            goto failed_with_unlock;
        }
    }

    if (udmabuf_get_quirk_mmap_property(dev, &quirk_mmap_mode, true) == 0)
        udmabuf_set_quirk_mmap_mode(obj, quirk_mmap_mode);

    if (of_property_read_bool(dev->of_node, "quirk-mmap-on"))
        udmabuf_set_quirk_mmap_mode(obj, QUIRK_MMAP_MODE_ALWAYS_ON);

    if (of_property_read_bool(dev->of_node, "quirk-mmap-off"))
        udmabuf_set_quirk_mmap_mode(obj, QUIRK_MMAP_MODE_ALWAYS_OFF);

    if (of_property_read_bool(dev->of_node, "quirk-mmap-auto"))
        udmabuf_set_quirk_mmap_mode(obj, QUIRK_MMAP_MODE_AUTO);

    if (of_property_read_bool(dev->of_node, "quirk-mmap-page"))
        udmabuf_set_quirk_mmap_mode(obj, QUIRK_MMAP_MODE_PAGE);

    if (of_property_read_u32(dev->of_node, "sync-mode", &u32_value) == 0) {
        if ((u32_value < SYNC_MODE_MIN) || (u32_value > SYNC_MODE_MAX)) {
            dev_err(dev, "invalid sync-mode property value = %d\n", u32_value);
            goto failed_with_unlock;
        }
        obj->sync_mode &= ~SYNC_MODE_MASK;
        obj->sync_mode |= (int)u32_value;
    }

    if (of_property_read_bool(dev->of_node, "sync-always"))
        obj->sync_mode |= SYNC_ALWAYS;

    if (of_property_read_u32(dev->of_node, "sync-direction", &u32_value) == 0) {
        if (u32_value > 2) {
            dev_err(dev, "invalid sync-direction property value = %d\n", u32_value);
            goto failed_with_unlock;
        }
        obj->sync_direction = (int)u32_value;
    }

    if (of_property_read_ulong(dev->of_node, "sync-offset", &u64_value) == 0) {
        if (u64_value >= obj->size) {
            dev_err(dev, "invalid sync-offset property value = %d\n", u64_value);
            goto failed_with_unlock;
        }
        obj->sync_offset = (int)u64_value;
    }

    if (of_property_read_ulong(dev->of_node, "sync-size", &u64_value) == 0) {
        if (obj->sync_offset + u64_value > obj->size) {
            dev_err(dev, "invalid sync-size property value = %d\n", u64_value);
            goto failed_with_unlock;
        }
        obj->sync_size = obj->size;
    }

    ret = udmabuf_object_setup(obj);
    if (ret) {
        dev_err(dev, "object setup failed. ret = %d\n", ret);
        goto failed_with_unlock;
    }

    mutex_unlock(&obj->sem);

    if (info_enable)
        udmabuf_object_info(obj);

    return 0;

failed_with_unlock:
    mutex_unlock(&obj->sem);
failed:
    if (obj != NULL)
        udmabuf_platform_device_remove(dev, obj);
    else
        dev_set_drvdata(dev, NULL);

    return ret;
}

static int udmabuf_platform_driver_probe(struct platform_device *pdev)
{
    int ret;

    ret = udmabuf_platform_device_probe(&pdev->dev);

    if (ret != 0) {
        dev_err(&pdev->dev, "driver probe failed, return = %d\n", ret);
    } else if (info_enable) {
        dev_info(&pdev->dev, "driver installed\n");
    }

    return ret;
}

static int udmabuf_platform_driver_remove(struct platform_device *pdev)
{
    struct udmabuf_object *obj = dev_get_drvdata(&pdev->dev);
    int ret;

    ret = udmabuf_platform_device_remove(&pdev->dev, obj);

    if (ret != 0) {
        dev_err(&pdev->dev, "driver remove failed, return = %d\n", ret);
    } else if (info_enable) {
        dev_info(&pdev->dev, "driver removed.\n");
    }

    return ret;
}

static struct of_device_id udmabuf_of_match[] = {
    { .compatible = "free,u-dma-buf", },
    {  }
};
MODULE_DEVICE_TABLE(of, udmabuf_of_match);

static struct platform_driver udmabuf_platform_driver = {
    .probe = udmabuf_platform_driver_probe,
    .remove = udmabuf_platform_driver_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = DRIVER_NAME,
        .of_match_table = udmabuf_of_match,
    },
};

static void udmabuf_device_list_delete_entry(struct udmabuf_device_entry *entry)
{
    mutex_lock(&udmabuf_device_list_sem);
    list_del(&entry->list);
    mutex_unlock(&udmabuf_device_list_sem);
    kfree(entry);
}

static void udmabuf_device_list_remove_entry(struct udmabuf_device_entry *entry)
{
    struct device *dev = entry->dev;
    struct device *parent = entry->parent;
    void (*prep_remove)(struct device *dev) = entry->prep_remove;
    void (*post_remove)(struct device *dev) = entry->post_remove;

    if (prep_remove)
        prep_remove(dev);
    udmabuf_device_list_delete_entry(entry);
    if (post_remove)
        post_remove(dev);
    if (parent)
        put_device(parent);
}

static void udmabuf_device_list_cleanup(void)
{
    struct udmabuf_device_entry *entry;
    while (!list_empty(&udmabuf_device_list)) {
        entry = list_first_entry(&udmabuf_device_list, typeof(*(entry)), list);
        udmabuf_device_list_remove_entry(entry);
    }
}

static void u_dma_buf_cleanup(void)
{
    udmabuf_device_list_cleanup();

    if (udmabuf_platform_driver_registered)
        platform_driver_unregister(&udmabuf_platform_driver);

    if (udmabuf_sys_class != NULL)
        class_destroy(udmabuf_sys_class);

    if (udmabuf_device_number != 0)
        unregister_chrdev_region(udmabuf_device_number, DEVICE_MAX_NUM);

    ida_destroy(&udmabuf_device_ida);
}

static int __init u_dma_buf_init(void)
{
    int ret;

    ida_init(&udmabuf_device_ida);
    INIT_LIST_HEAD(&udmabuf_device_list);
    mutex_init(&udmabuf_device_list_sem);

    ret = alloc_chrdev_region(&udmabuf_device_num, 0, DEVICE_MAX_NUM, DRIVER_NAME);
    if (ret != 0) {
        pr_err(DRIVER_NAME ": couldn't allocate device major number, ret = %d\n", ret);
        udmabuf_device_number = 0;
        goto failed;
    }

    udmabuf_sys_class = class_create(DRIVER_NAME);
    if (IS_ERR_OR_NULL(udmabuf_sys_class)) {
        ret = PTR_ERR(udmabuf_sys_class);
        udmabuf_sys_class = NULL;
        pr_err(DRIVER_NAME ": couldn't create sys class. ret = %d\n", ret);
        ret = (ret == 0) ? -ENOMEM : ret;
        goto failed;
    }

    udmabuf_sys_class_set_attribute();

    udmabuf_static_device_reserve_minor_number_all();

    ret = platform_driver_register(&udmabuf_platform_driver);
    if (ret) {
        pr_err(DRIVER_NAME ": couldn't register platform driver\n");
        udmabuf_platform_driver_registered = false;
        goto failed;
    } else {
        udmabuf_platform_driver_registered = true;
    }

    ret = udmabuf_static_device_create_all();
    if (ret) {
        pr_err(DRIVER_NAME ": couldn't create static devices\n");
        goto failed;
    }

    return 0;

failed:
    u_dma_buf_cleanup();
    return ret;
}

static void __exit u_dma_buf_exit(void)
{
    u_dma_buf_cleanup();
}

module_init(u_dma_buf_init);
module_exit(u_dma_buf_exit);

MODULE_DESCRIPTION("User space mappable DMA buffer device driver");
MODULE_AUTHOR("yinwg hkdywg@163.com");
MODULE_LICENSE("GPL");
