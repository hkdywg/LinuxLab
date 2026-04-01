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
#include <linux/property.h>
#include <linux/dma-mapping.h>
#include <linux/dma-noncoherent.h>

#include <linux/of_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/dma-buf.h>
#include <linux/dma-direct.h>
#include <linux/iommu.h>
#include <linux/ioctl.h>

#define DRIVER_NAME         "u-dma-buf"
#define DEVICE_NAME_FORMAT  "udmabuf%d"
#define DRIVER_VERSION      "V1.0.0"
#define DEVICE_MAX_NUM      256


/*
 * info_enable module parameter
 */
static int info_enable = 1;
module_param(info_enable, int , S_IRUGO);
MODULE_PARM_DESC(info_enable, "udmabuf install/uninstall information enable");
#define DMA_INFO_ENABLE     (info_enable & 0x02)

/*
 * dma_mask_bit module parameter
 */
static int dma_mask_bit = 32;
module_param(dma_mask_bit, int , S_IRUGO);
MODULE_PARM_DESC(dma_mask_bit, "udmabuf dma mask bit(default=32)");

/*
 * bind module parameter
 */
static char *bind = NULL;
module_param(bind, charp, S_IRUGO);
MODULE_PARM_DESC(bind, "bind device name. exp pci/0000:00:20:0");

/*
 * quirk_mmap_mode module parameter
 */
#define QUIRK_MMAP_MODE_UNDEFINED   0
#define QUIRK_MMAP_MODE_ALWAYS_OFF  1
#define QUIRK_MMAP_MODE_ALWAYS_ON   2
#define QUIRK_MMAP_MODE_AUTO        3
#define QUIRK_MMAP_MODE_PAGE        4
#define QUIRK_MMAP_MODE_PARAM_DESC_USAGE    "(1:off,2:on,3:auto,4:page)"
#define QUIRK_MMAP_MODE_PARAM_DESC_DEFAULT  "(default=3)"
static int quirk_mmap_mode = QUIRK_MMAP_MODE_AUTO;
module_param(quirk_mmap_mode, int, S_IRUGO);
MODULE_PARM_DESC(quirk_mmap_mode, "udmabuf default quirk mmap mode" QUIRK_MMAP_MODE_PARAM_DESC_USAGE QUIRK_MMAP_MODE_PARAM_DESC_DEFAULT);

/*
 * udmabuf device entry structure
 */
struct udmabuf_device_entry {
    struct device *dev;
    struct device *parent;
    void (*prep_remove)(struct device *dev);
    void (*post_remove)(struct device *dev);
    struct list_head list;
};

/*
 * udmabuf object structure
 */
struct udmabuf_object {
    struct device *sys_dev;
    struct device *dma_dev;
    struct cdev cdev;
    dev_t device_number;
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

/*
 * udmabuf export entry structure
 */
struct udmabuf_export_entry {
    struct udmabuf_object *object;
    struct udmabuf_object object_data;
    struct dma_buf *dma_buf;
    int fd;
    bool force_sync;
    u64 offset;
    size_t size;
    struct list_head list;
};


#define UDMABUF_VMA_DEBUG(obj,bit) ((obj->debug_vma & (1<<bit)) != 0)

/*
 * sync_mode(synchronous mode) value
 */
#define SYNC_MODE_INVALID           (0x00)
#define SYNC_MODE_NONCACHED         (0x01)
#define SYNC_MODE_WRITECOMBINE      (0x02)
#define SYNC_MODE_DMACHOHERENT      (0x03)
#define SYNC_MODE_MASK              (0x03)
#define SYNC_MODE_MIN               (0x01)
#define SYNC_MODE_MAX               (0x03)
#define SYNC_ALWAYS                 (0x04)

/*
 * sync command
 */
#define SYNC_COMMAND_DIR_MASK       (0x000000000000000C)
#define SYNC_COMMAND_DIR_SHIFT      (2)
#define SYNC_COMMAND_SIZE_MASK      (0x00000000FFFFFFF0)
#define SYNC_COMMAND_SIZE_SHIFT     (0)
#define SYNC_COMMAND_OFFSET_MASK    (0xFFFFFFFF00000000)
#define SYNC_COMMAND_OFFSET_SHIFT   (32)
#define SYNC_COMMAND_ARGMENT_MASK   (0xFFFFFFFFFFFFFFFE)

/**
 * DOC: Udmabuf System Class Device File Description.
 *
 * This section define the device file created in system class when udmabuf is 
 * loaded into the kernel.
 *
 * The device file created in system class is as follows.
 *
 * * /sys/class/u-dma-buf/<device-name>/driver_version
 * * /sys/class/u-dma-buf/<device-name>/phys_addr
 * * /sys/class/u-dma-buf/<device-name>/size
 * * /sys/class/u-dma-buf/<device-name>/sync_mode
 * * /sys/class/u-dma-buf/<device-name>/sync_offset
 * * /sys/class/u-dma-buf/<device-name>/sync_size
 * * /sys/class/u-dma-buf/<device-name>/sync_direction
 * * /sys/class/u-dma-buf/<device-name>/sync_owner
 * * /sys/class/u-dma-buf/<device-name>/sync_for_cpu
 * * /sys/class/u-dma-buf/<device-name>/sync_for_device
 * * /sys/class/u-dma-buf/<device-name>/dma_coherent
 * * /sys/class/u-dma-buf/<device-name>/quirk_mmap_mode
 * * /sys/class/u-dma-buf/<device-name>/ioctl_version
 * * 
 */

static int udmabuf_sync_command_arguments(struct udmabuf_object *obj, u64 command, dma_addr_t *phys_addr,
                                        size_t *size, enum dma_data_direction *direction)
{
    u64 sync_offset;
    size_t sync_size;
    int sync_direction;

    if ((command & SYNC_COMMAND_ARGMENT_MASK) != 0) {
        sync_offset     = (u64   )((command & SYNC_COMMAND_OFFSET_MASK) >> SYNC_COMMAND_OFFSET_SHIFT);
        sync_size       = (size_t)((command & SYNC_COMMAND_SIZE_MASK) >> SYNC_COMMAND_SIZE_SHIFT);
        sync_direction  = (int   )((command & SYNC_COMMAND_DIR_MASK) >> SYNC_COMMAND_DIR_SHIFT);
    } else {
        sync_offset = obj->sync_offset;
        sync_size = obj->sync_size;
        sync_direction = obj->sync_direction;
    }

    if (sync_offset + sync_size > obj->size)
        return -EINVAL;

    switch (sync_direction) {
    case 1: *direction = DMA_TO_DEVICE; break;
    case 2: *direction = DMA_FROM_DEVICE; break;
    default: *direction = DMA_BIDIRECTIONAL; break;
    }

    *phys_addr = obj->phys_addr + sync_offset;
    *size = sync_size;

    return 0;
}

static int udmabuf_sync_for_cpu(struct udmabuf_object *obj)
{
    int ret = 0;

    if (obj->sync_for_cpu) {
        dma_addr_t phys_addr;
        size_t size;
        enum dma_data_direction direction;
        ret = udmabuf_sync_command_arguments(obj, obj->sync_for_cpu, &phys_addr, &size, &direction);
        if (ret == 0) {
            dma_sync_single_for_cpu(obj->dma_dev, phys_addr, size, direction);
            obj->sync_for_cpu = 0;
            obj->sync_owner = 0;
        }
    }

    return ret;
}

static int udmabuf_sync_for_device(struct udmabuf_object *obj)
{
    int ret = 0;

    if (obj->sync_for_device) {
        dma_addr_t phys_addr;
        size_t size;
        enum dma_data_direction direction;
        ret = udmabuf_sync_command_arguments(obj, obj->sync_for_cpu, &phys_addr, &size, &direction);
        if (ret == 0) {
            dma_sync_single_for_device(obj->dma_dev, phys_addr, size, direction);
            obj->sync_for_device = 0;
            obj->sync_owner = 1;
        }
    }

    return ret;
}

static struct class *udmabuf_sys_class = NULL;
static bool udmabuf_platform_driver_registered = false;

static DEFINE_IDA(udmabuf_device_ida);
static dev_t udmabuf_device_number = 0;
static struct list_head udmabuf_device_list;
static struct mutex udmabuf_device_list_sem;

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

DEF_ATTR_SHOW(size            , "%zu\n"     ,    obj->size                     );
DEF_ATTR_SHOW(phys_addr       , "0x%llx\n"  ,    obj->phys_addr                );
DEF_ATTR_SHOW(sync_mode       , "%d\n"      ,    obj->sync_mode                );
DEF_ATTR_SHOW(sync_offset     , "0x%llx\n"  ,    obj->sync_offset              );
DEF_ATTR_SHOW(sync_size       , "%zu\n"     ,    obj->sync_size                );
DEF_ATTR_SHOW(sync_direction  , "%d\n"      ,    obj->sync_direction           );
DEF_ATTR_SHOW(sync_owner      , "%d\n"      ,    obj->sync_owner               );
DEF_ATTR_SHOW(sync_for_cpu    , "%llu\n"    ,    obj->sync_for_cpu             );
DEF_ATTR_SHOW(sync_for_device , "%llu\n"    ,    obj->sync_for_device          );
DEF_ATTR_SHOW(quirk_mmap_mode , "%d\n"      ,    obj->quirk_mmap_mode          );
DEF_ATTR_SHOW(dma_coherent    , "%d\n"      ,    dev_is_dma_coherent(obj->dma_dev) );
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

#define udmabuf_device_attrs_size (sizeof(udmabuf_device_attrs)/sizeof(udmabuf_device_attrs[0]))

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

static void udmabuf_mmap_vma_open(struct vm_area_struct *vma)
{
    struct udmabuf_object *obj = vma->vm_private_data;
    if (UDMABUF_VMA_DEBUG(obj, 0))
        dev_info(obj->dma_dev, "%s(virt_addr = 0x%lx, offset = 0x%lx, flags = 0x%lx)\n",
                __func__, vma->vm_start, vma->vm_pgoff << PAGE_SHIFT, vma->vm_flags);
}

static void udmabuf_mmap_vma_close(struct vm_area_struct *vma)
{
    struct udmabuf_object *obj = vma->vm_private_data;
    if (UDMABUF_VMA_DEBUG(obj, 0))
        dev_info(obj->dma_dev, "%s(virt_addr = 0x%lx, offset = 0x%lx, flags = 0x%lx)\n",
                __func__, vma->vm_start, vma->vm_pgoff << PAGE_SHIFT, vma->vm_flags);
}

static vm_fault_t udmabuf_mmap_vma_fault(struct vm_fault *vmf)
{
    struct vm_area_struct *vma = vmf->vma;
    struct udmabuf_object *obj = vma->vm_private_data;
    unsigned long offset = vmf->pgoff << PAGE_SHIFT;
    unsigned long phys_addr = obj->phys_addr + offset;
    unsigned long page_frame_num = phys_addr >> PAGE_SHIFT;
    unsigned long request_size = 1UL << PAGE_SHIFT;
    unsigned long available_size = obj->alloc_size - offset;
    unsigned long virt_addr = vmf->address;

    if (UDMABUF_VMA_DEBUG(obj, 1))
        dev_info(obj->dma_dev, "vma_fault(virt_addr = %pad, phys_addr = %pad)\n",
                 &virt_addr, &phys_addr);

    if (request_size > available_size)
        return VM_FAULT_SIGBUS;

    if (!pfn_valid(page_frame_num))
        return VM_FAULT_SIGBUS;

    if (obj->pages != NULL) {
        if (vmf->pgoff >= obj->pagecount) 
            return VM_FAULT_SIGBUS;
        return vmf_insert_page(vma, virt_addr, obj->pages[vmf->pgoff]);
    }

    return vmf_insert_pfn(vma, virt_addr, page_frame_num);
}

static const struct vm_operations_struct udmabuf_mmap_vm_ops = {
    .open = udmabuf_mmap_vma_open,
    .close = udmabuf_mmap_vma_close,
    .fault = udmabuf_mmap_vma_fault,
};

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
                                                        
static bool udmabuf_quirk_mmap_enable(struct udmabuf_object *obj)
{
    if (!obj)
        return true;

    if (obj->quirk_mmap_mode == QUIRK_MMAP_MODE_PAGE)
        return true;
    if (obj->quirk_mmap_mode == QUIRK_MMAP_MODE_ALWAYS_OFF)
        return false;
    if (obj->quirk_mmap_mode == QUIRK_MMAP_MODE_ALWAYS_ON)
        return true;
    if (obj->quirk_mmap_mode == QUIRK_MMAP_MODE_AUTO)
        return !dev_is_dma_coherent(obj->dma_dev);

    return true;
}

static inline void vm_flags_mod(struct vm_area_struct *vma, vm_flags_t set, vm_flags_t clear)
{
    vma->vm_flags |= (set);
    vma->vm_flags &= ~(clear);
}

static int udmabuf_object_mmap(struct udmabuf_object *obj, struct vm_area_struct *vma, bool force_sync)
{
    if (vma->vm_pgoff + vma_pages(vma) > (obj->alloc_size >> PAGE_SHIFT))
        return -EINVAL;

    if ((force_sync == true) || (obj->sync_mode & SYNC_ALWAYS) != 0) {
        switch (obj->sync_mode & SYNC_MODE_MASK) {
        case SYNC_MODE_NONCACHED:
            vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
            break;
        case SYNC_MODE_WRITECOMBINE:
            vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
            break;
        case SYNC_MODE_DMACHOHERENT:
            vma->vm_page_prot  = pgprot_writecombine(vma->vm_page_prot);
            break;
        default:
            break;
        }
    }

    vm_flags_mod(vma, (VM_IO | VM_PFNMAP | VM_DONTEXPAND | VM_DONTDUMP), 0);

    if (udmabuf_quirk_mmap_enable(obj)) {
        unsigned long page_frame_num = (obj->phys_addr >> PAGE_SHIFT) + vma->vm_pgoff;
        if (obj->pages != NULL) {
            vm_flags_mod(vma, VM_MIXEDMAP, (VM_PFNMAP | VM_IO | VM_DONTEXPAND));
            vma->vm_ops = &udmabuf_mmap_vm_ops;
            vma->vm_private_data = obj;
            udmabuf_mmap_vma_open(vma);
            return 0;
        }
        if (pfn_valid(page_frame_num)) {
            vma->vm_ops = &udmabuf_mmap_vm_ops;
            vma->vm_private_data = obj;
            udmabuf_mmap_vma_open(vma);
            return 0;
        }
    }

    return dma_mmap_coherent(obj->dma_dev, vma, obj->virt_addr, obj->phys_addr, obj->alloc_size);
}

static struct sg_table *udmabuf_export_dma_buf_map(struct dma_buf_attachment *attachment,
                                                enum dma_data_direction direction)
{
    struct dma_buf *dma_buf = attachment->dmabuf;
    struct udmabuf_export_entry *entry;
    struct udmabuf_object *obj;
    unsigned int done  = 0;
    const unsigned int DONE_ALLOC_SG_TABLE = (1 << 0);
    const unsigned int DONE_GET_SG_TABLE = (1 << 1);
    const unsigned int DONE_MAP_SG_TABLE = (1 << 2);
    struct sg_table *sg_table;
    int ret = 0;

    if (dma_buf == NULL)
        return ERR_PTR(-ENODEV);

    if ((entry = dma_buf->priv) == NULL)
        return ERR_PTR(-ENODEV);

    obj = &entry->object_data;

    sg_table = kzalloc(sizeof(*sg_table), GFP_KERNEL);
    if (IS_ERR_OR_NULL(sg_table)) {
        dev_err(obj->sys_dev, "%s(fd=%d): kzalloc failed. return=%ld\n", __func__, entry->fd, PTR_ERR(sg_table));
        goto failed;
    }
    done |= DONE_ALLOC_SG_TABLE;

    ret = dma_get_sgtable(obj->dma_dev, sg_table, obj->virt_addr, obj->phys_addr, obj->alloc_size);
    if (ret) {
        dev_err(obj->sys_dev, "%s(fd=%d): dma_get_sgtable failed. return=%d\n", __func__, entry->fd, ret);
        goto failed;
    }
    done |= DONE_GET_SG_TABLE;

    ret = dma_map_sg_attrs(attachment->dev, sg_table->sgl, sg_table->orig_nents, direction, 0);
    if (ret) {
        dev_err(obj->sys_dev, "%s(fd=%d): dma_map_sgtable failed. return=%d\n", __func__, entry->fd, ret);
        goto failed;
    }
    sg_table->nents = ret;
    done |= DONE_MAP_SG_TABLE;

    return  sg_table;

failed:
    if (done & DONE_MAP_SG_TABLE) { dma_unmap_sg_attrs(attachment->dev, sg_table->sgl, sg_table->orig_nents, direction, 0); }
    if (done & DONE_GET_SG_TABLE) { sg_free_table(sg_table); }
    if (done & DONE_ALLOC_SG_TABLE) { kfree(sg_table); }
    
    return ERR_PTR(ret);
}

static void udmabuf_export_dma_buf_unmap(struct dma_buf_attachment *attachment,
                        struct sg_table *sg_table, enum dma_data_direction direction)
{
    struct dma_buf *dma_buf = attachment->dmabuf;

    if (dma_buf->priv == NULL || sg_table == NULL)
        return;

    dma_unmap_sg_attrs(attachment->dev, sg_table->sgl, sg_table->orig_nents, direction, 0);
    sg_free_table(sg_table);
    kfree(sg_table);
}

static void udmabuf_export_release(struct dma_buf *dma_buf)
{
    struct udmabuf_export_entry *entry = dma_buf->priv;
    struct udmabuf_object *obj;

    if (entry == NULL || (obj = entry->object) == NULL)
        return;

    mutex_lock(&obj->export_dma_buf_list_sem);
    list_del(&entry->list);
    mutex_unlock(&obj->export_dma_buf_list_sem);
    kfree(entry);
}

static int udmabuf_export_mmap(struct dma_buf *dma_buf, struct vm_area_struct *vma)
{
    struct udmabuf_export_entry *entry = dma_buf->priv;
    struct udmabuf_object *obj;
    int ret;

    if (entry == NULL)
        return -ENODEV;

    obj = &entry->object_data;

    ret = udmabuf_object_mmap(obj, vma, entry->force_sync);
    if (ret) {
        dev_err(obj->sys_dev, "%s(fd=%d): udmabuf_object_mmap failed. return=%d\n", __func__, entry->fd, ret);
        return ret;
    }

    return 0;
}

static int udmabuf_export_begin_cpu(struct dma_buf *dma_buf, enum dma_data_direction direction)
{
    struct udmabuf_export_entry *entry = dma_buf->priv;
    struct udmabuf_object *obj;

    if (entry == NULL)
        return -ENODEV;

    obj = &entry->object_data;

    obj->sync_for_cpu = 1;
    obj->sync_offset = 0;
    obj->sync_size = obj->alloc_size;
    obj->sync_direction = direction;
    udmabuf_sync_for_cpu(obj);

    return 0;
}

static int udmabuf_export_end_cpu(struct dma_buf *dma_buf, enum dma_data_direction direction)
{
    struct udmabuf_export_entry *entry = dma_buf->priv;
    struct udmabuf_object *obj;

    if (entry == NULL)
        return -ENODEV;

    obj = &entry->object_data;

    obj->sync_for_device = 1;
    obj->sync_offset = 0;
    obj->sync_size = obj->alloc_size;
    obj->sync_direction = direction;
    udmabuf_sync_for_device(obj);

    return 0;
}

static void *udmabuf_export_kmap(struct dma_buf *dma_buf, unsigned long page)
{
    return NULL;
}

static const struct dma_buf_ops udmabuf_export_ops = {
    .map                = udmabuf_export_kmap,
    .map_dma_buf        = udmabuf_export_dma_buf_map,
    .unmap_dma_buf      = udmabuf_export_dma_buf_unmap,
    .release            = udmabuf_export_release,
    .mmap               = udmabuf_export_mmap,
    .begin_cpu_access   = udmabuf_export_begin_cpu,
    .end_cpu_access     = udmabuf_export_end_cpu,
};

static struct udmabuf_export_entry *udmabuf_export_create_entry(struct udmabuf_object *obj,
                                    u64 offset, size_t size, unsigned long fd_flags)
{
    DEFINE_DMA_BUF_EXPORT_INFO(export_info);
    struct udmabuf_export_entry *entry = NULL;
    bool force_sync;
    unsigned long export_fd_flags, dmabuf_fd_flags;
    int ret = 0;

    if ((offset & (PAGE_SIZE-1)) != 0) {
        dev_err(obj->sys_dev, "%s offset is not page allignment\n", __func__);
        ret = -EINVAL;
        goto failed;
    }

    if ((size & (PAGE_SIZE-1)) != 0) {
        dev_err(obj->sys_dev, "%s size is not page allignment\n", __func__);
        ret = -EINVAL;
        goto failed;
    }

    if (offset + size > obj->size) {
        dev_err(obj->sys_dev, "%s offset+size is over buffer size\n", __func__);
        ret = -EINVAL;
        goto failed;
    }

    if ((fd_flags & ~(O_CLOEXEC | O_SYNC | O_ACCMODE)) != 0) {
        dev_err(obj->sys_dev, "%s invalid fd_flags\n", __func__);
        ret = -EINVAL;
        goto failed;
    }
    force_sync = ((fd_flags & O_SYNC) != 0);
    export_fd_flags = ((fd_flags & ~O_SYNC));
    dmabuf_fd_flags = ((fd_flags & ~O_SYNC));

    entry = kzalloc(sizeof(*entry), GFP_KERNEL);
    if (IS_ERR_OR_NULL(entry)) {
        ret = PTR_ERR(entry);
        entry = NULL;
        dev_err(obj->sys_dev, "%s kzalloc failed. return=%d\n", __func__, ret);
        goto failed;
    }
    entry->object = obj;
    entry->offset = offset;
    entry->size = size;

    entry->object_data.sys_dev = obj->sys_dev;
    entry->object_data.dma_dev = obj->dma_dev;
    entry->object_data.phys_addr = obj->phys_addr + offset;
    entry->object_data.virt_addr = obj->virt_addr + offset;
    entry->object_data.size = size;
    entry->object_data.alloc_size = size;
    entry->object_data.sync_mode = obj->sync_mode;
    entry->object_data.sync_offset = 0;
    entry->object_data.sync_size = 0;
    entry->object_data.sync_direction = 0;
    entry->force_sync = force_sync;
    entry->object_data.quirk_mmap_mode = obj->quirk_mmap_mode;
    if (obj->pages != NULL) {
        entry->object_data.pagecount = size >> PAGE_SHIFT;
        entry->object_data.pages = &obj->pages[offset>>PAGE_SHIFT];
    }
    entry->object_data.debug_vma = obj->debug_vma;
    entry->object_data.debug_export = obj->debug_export;

    export_info.ops = &udmabuf_export_ops;
    export_info.size = size;
    export_info.priv = (void *)entry;
    export_info.flags = export_fd_flags;
    export_info.exp_name = dev_name(obj->sys_dev);

    entry->dma_buf = dma_buf_export(&export_info);
    if (IS_ERR(entry->dma_buf)) {
        dev_err(obj->sys_dev, "%s: dma_buf_export failed. return=%ld\n", __func__, PTR_ERR(entry->dma_buf));
        entry->dma_buf = NULL;
        goto failed;
    }

    entry->fd = dma_buf_fd(entry->dma_buf, dmabuf_fd_flags);
    if (entry->fd < 0) {
        dev_err(obj->sys_dev, "%s: dma_buf_fd failed. return=%d\n", __func__, entry->fd);
        entry->fd = 0;
        goto failed;
    }
    
    mutex_lock(&obj->export_dma_buf_list_sem);
    list_add_tail(&entry->list, &obj->export_dma_buf_list);
    mutex_unlock(&obj->export_dma_buf_list_sem);

    return entry;

failed:
    if (entry != NULL) {
        if (entry->dma_buf != NULL)
            dma_buf_put(entry->dma_buf);
        kfree(entry);
    }
    return ERR_PTR(ret);
}

static int udmabuf_device_file_open(struct inode *inode, struct file *file)
{
    struct udmabuf_object *obj;

    obj = container_of(inode->i_cdev, struct udmabuf_object, cdev);
    file->private_data = obj;
    obj->is_open = 1;

    return 0;
}

static int udmabuf_device_file_release(struct inode *inode, struct file *file)
{
    struct udmabuf_object *obj = file->private_data;

    obj->is_open = 0;

    return 0;
}

static int udmabuf_device_file_mmap(struct file *file, struct vm_area_struct *vma)
{
    struct udmabuf_object *obj = file->private_data;
    bool force_sync = ((file->f_flags & O_SYNC) != 0);

    return udmabuf_object_mmap(obj, vma, force_sync);
}

static ssize_t udmabuf_device_file_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
    struct udmabuf_object *obj = file->private_data;
    int ret;
    size_t xfer_size, remain_size;
    dma_addr_t phys_addr;
    void *virt_addr;
    bool need_sync;

    if (mutex_lock_interruptible(&obj->sem))
        return -ERESTARTSYS;

    if (*pos >= obj->size) {
        ret = 0;
        goto return_unlock;
    }

    phys_addr = obj->phys_addr + *pos;
    virt_addr = obj->virt_addr + *pos;
    xfer_size = (*pos + count >= obj->size) ? obj->size - *pos : count;
    need_sync = (((file->f_flags & O_SYNC) != 0) || ((obj->sync_mode & SYNC_ALWAYS) != 0));

    if (need_sync == true)
        dma_sync_single_for_cpu(obj->dma_dev, phys_addr, xfer_size, DMA_FROM_DEVICE);

    if ((remain_size = copy_to_user(buf, virt_addr, xfer_size)) != 0) {
        ret = 0;
        goto return_unlock;
    }

    if (need_sync == true)
        dma_sync_single_for_device(obj->dma_dev, phys_addr, xfer_size, DMA_FROM_DEVICE);

    *pos += xfer_size;
    ret = xfer_size;

return_unlock:
    mutex_unlock(&obj->sem);
    return ret;
}

static ssize_t udmabuf_device_file_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
    struct udmabuf_object *obj = file->private_data;
    int ret;
    size_t xfer_size, remain_size;
    dma_addr_t phys_addr;
    void *virt_addr;
    bool need_sync;

    if (mutex_lock_interruptible(&obj->sem))
        return -ERESTARTSYS;

    if (*pos >= obj->size) {
        ret = 0;
        goto return_unlock;
    }

    phys_addr = obj->phys_addr + *pos;
    virt_addr = obj->virt_addr + *pos;
    xfer_size = (*pos + count >= obj->size) ? obj->size - *pos : count;
    need_sync = (((file->f_flags & O_SYNC) != 0) || ((obj->sync_mode & SYNC_ALWAYS) != 0));

    if (need_sync == true)
        dma_sync_single_for_cpu(obj->dma_dev, phys_addr, xfer_size, DMA_TO_DEVICE);

    if ((remain_size = copy_from_user(virt_addr, buf, xfer_size)) != 0) {
        ret = 0;
        goto return_unlock;
    }

    if (need_sync == true)
        dma_sync_single_for_device(obj->dma_dev, phys_addr, xfer_size, DMA_TO_DEVICE);

    *pos += xfer_size;
    ret = xfer_size;

return_unlock:
    mutex_unlock(&obj->sem);
    return ret;
}

static loff_t udmabuf_device_file_llseek(struct file *file, loff_t offset, int whence)
{
    struct udmabuf_object *obj = file->private_data;
    loff_t new_pos;

    switch (whence) {
    case 0:
        new_pos = offset;
        break;
    case 1:
        new_pos = file->f_pos + offset;
        break;
    case 2:
        new_pos = obj->size + offset;
        break;
    default:
        return -EINVAL;
    }
    if (new_pos < 0) return -EINVAL;
    if (new_pos > obj->size) return -EINVAL;
    file->f_pos = new_pos;

    return new_pos;
}


#define DEFINE_U_DMA_BUF_IOCTL_FLAGS(name,type,lo,hi)                   \
static const int U_DMA_BUF_IOCTL_FLAGS_ ## name ## _SHIFT = (lo);       \
static const uint64_t U_DMA_BUF_IOCTL_FLAGS_ ## name ## _MASK = (((uint64_t)1UL << ((hi)-(lo)+1))-1);   \
static inline void SET_U_DMA_BUF_IOCTL_FLAGS_ ## name(type *p, int value)   \
{                                                                       \
    const int shift = U_DMA_BUF_IOCTL_FLAGS_ ## name ## _SHIFT;         \
    const uint64_t mask = U_DMA_BUF_IOCTL_FLAGS_ ## name ## _MASK;      \
    p->flags &= ~(mask << shift);                                       \
    p->flags |= ((value & mask) << shift);                              \
}                                                                       \
static inline int GET_U_DMA_BUF_IOCTL_FLAGS_ ## name(type *p)           \
{                                                                       \
    const int shift = U_DMA_BUF_IOCTL_FLAGS_ ## name ## _SHIFT;         \
    const uint64_t mask = U_DMA_BUF_IOCTL_FLAGS_ ## name ## _MASK;      \
    return (int)((p->flags >> shift) & mask);                           \
}

typedef struct {
    uint64_t flags;
    char version[16];
} u_dma_buf_ioctl_drv_info;

DEFINE_U_DMA_BUF_IOCTL_FLAGS(IOCTL_VERSION,         u_dma_buf_ioctl_drv_info,  0,  7)
DEFINE_U_DMA_BUF_IOCTL_FLAGS(IN_KERNEL_FUNCTIONS,   u_dma_buf_ioctl_drv_info,  8,  8)
DEFINE_U_DMA_BUF_IOCTL_FLAGS(USE_OF_DMA_CONFIG,     u_dma_buf_ioctl_drv_info, 12, 12)
DEFINE_U_DMA_BUF_IOCTL_FLAGS(USE_OF_RESERVED_MEM,   u_dma_buf_ioctl_drv_info, 13, 13)
DEFINE_U_DMA_BUF_IOCTL_FLAGS(USE_QUIRK_MMAP,        u_dma_buf_ioctl_drv_info, 16, 16)
DEFINE_U_DMA_BUF_IOCTL_FLAGS(USE_QUIRK_MMAP_PAGE,   u_dma_buf_ioctl_drv_info, 17, 17)

typedef struct {
    uint64_t flags;
    uint64_t size;
    uint64_t addr;
} u_dma_buf_ioctl_dev_info;

DEFINE_U_DMA_BUF_IOCTL_FLAGS(DMA_MASK,      u_dma_buf_ioctl_dev_info,  0,  7)
DEFINE_U_DMA_BUF_IOCTL_FLAGS(DMA_COHERENT,  u_dma_buf_ioctl_dev_info,  9,  9)
DEFINE_U_DMA_BUF_IOCTL_FLAGS(MMAP_MODE,     u_dma_buf_ioctl_dev_info, 10, 12)

typedef struct {
    uint64_t flags;
    uint64_t size;
    uint64_t offset;
} u_dma_buf_ioctl_sync_args;

DEFINE_U_DMA_BUF_IOCTL_FLAGS(SYNC_CMD,      u_dma_buf_ioctl_sync_args,  0,  1)
DEFINE_U_DMA_BUF_IOCTL_FLAGS(SYNC_DIR,      u_dma_buf_ioctl_sync_args,  2,  3)
DEFINE_U_DMA_BUF_IOCTL_FLAGS(SYNC_MODE,     u_dma_buf_ioctl_sync_args,  8, 15)
DEFINE_U_DMA_BUF_IOCTL_FLAGS(SYNC_OWNER,    u_dma_buf_ioctl_sync_args, 16, 16)

enum {
    U_DMA_BUF_IOCTL_FLAGS_SYNC_CMD_FOR_CPU = 1,
    U_DMA_BUF_IOCTL_FLAGS_SYNC_CMD_FOR_DEVICE = 3
};

typedef struct {
    uint64_t flags;
    uint64_t size;
    uint64_t offset;
    uint64_t addr;
    int fd;
} u_dma_buf_ioctl_export_args;

DEFINE_U_DMA_BUF_IOCTL_FLAGS(EXPORT_FD_FLAGS,  u_dma_buf_ioctl_export_args, 0, 31)

#define U_DMA_BUF_IOCTL_MAGIC               'U'
#define U_DMA_BUF_IOCTL_GET_DRV_INFO        _IOR (U_DMA_BUF_IOCTL_MAGIC, 1, u_dma_buf_ioctl_drv_info)
#define U_DMA_BUF_IOCTL_GET_SIZE            _IOR (U_DMA_BUF_IOCTL_MAGIC, 2, uint64_t)
#define U_DMA_BUF_IOCTL_GET_DMA_ADDR        _IOR (U_DMA_BUF_IOCTL_MAGIC, 3, uint64_t)
#define U_DMA_BUF_IOCTL_GET_SYNC_OWNER      _IOR (U_DMA_BUF_IOCTL_MAGIC, 4, uint32_t)
#define U_DMA_BUF_IOCTL_SET_SYNC_FOR_CPU    _IOR (U_DMA_BUF_IOCTL_MAGIC, 5, uint64_t)
#define U_DMA_BUF_IOCTL_SET_SYNC_FOR_DEVICE _IOR (U_DMA_BUF_IOCTL_MAGIC, 6, uint64_t)
#define U_DMA_BUF_IOCTL_GET_DEV_INFO        _IOR (U_DMA_BUF_IOCTL_MAGIC, 7, u_dma_buf_ioctl_dev_info)
#define U_DMA_BUF_IOCTL_GET_SYNC            _IOR (U_DMA_BUF_IOCTL_MAGIC, 8, u_dma_buf_ioctl_sync_args)
#define U_DMA_BUF_IOCTL_SET_SYNC            _IOR (U_DMA_BUF_IOCTL_MAGIC, 9, u_dma_buf_ioctl_sync_args)
#define U_DMA_BUF_IOCTL_EXPORT              _IOR (U_DMA_BUF_IOCTL_MAGIC,10, u_dma_buf_ioctl_export_args)

static long udmabuf_device_file_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct udmabuf_object *obj = file->private_data;
    void __user *argp = (void __user*)arg;
    int ret;
    
    switch (cmd) {
    case U_DMA_BUF_IOCTL_GET_DRV_INFO: {
        u_dma_buf_ioctl_drv_info drv_info = {0};
        SET_U_DMA_BUF_IOCTL_FLAGS_IN_KERNEL_FUNCTIONS(&drv_info, 1);
        SET_U_DMA_BUF_IOCTL_FLAGS_USE_OF_DMA_CONFIG  (&drv_info, 1);
        SET_U_DMA_BUF_IOCTL_FLAGS_USE_OF_RESERVED_MEM(&drv_info, 1);
        SET_U_DMA_BUF_IOCTL_FLAGS_USE_QUIRK_MMAP     (&drv_info, 1);
        SET_U_DMA_BUF_IOCTL_FLAGS_USE_QUIRK_MMAP_PAGE(&drv_info, 1);
        if (strscpy(&drv_info.version[0], DRIVER_VERSION, sizeof(drv_info.version)) < 0)
            ret = -EFAULT;
        else if (copy_to_user(argp, &drv_info, sizeof(drv_info)) != 0)
            ret = -EFAULT;
        else 
            ret = 0;
        break;
        } 
    case U_DMA_BUF_IOCTL_GET_SIZE: {
        uint64_t size = (uint64_t)obj->size;
        if (copy_to_user(argp, &size, sizeof(size)) != 0)
            ret = -EFAULT;
        else
            ret = 0;
        break;
        }
    case U_DMA_BUF_IOCTL_GET_DMA_ADDR: {
        uint64_t dma_addr = (uint64_t)obj->phys_addr;
        if (copy_to_user(argp, &dma_addr, sizeof(dma_addr)) != 0)
            ret = -EFAULT;
        else 
            ret = 0;
        break;
        }
    case U_DMA_BUF_IOCTL_GET_SYNC_OWNER: {
        uint32_t sync_owner = (uint32_t)obj->sync_owner;
        if (copy_to_user(argp, &sync_owner, sizeof(sync_owner)) != 0)
            ret = -EFAULT;
        else
            ret = 0;
        break;
        }
    case U_DMA_BUF_IOCTL_GET_DEV_INFO: {
        u_dma_buf_ioctl_dev_info dev_info = {0};
        u64 dma_mask = (obj->dma_dev->dma_mask != NULL) ? *obj->dma_dev->dma_mask : 0;
        int dma_mask_size = 0;
        u64 dma_mask_bit = ((u64)1UL << dma_mask_size);
        while (dma_mask_size < 64) {
            if ((dma_mask & dma_mask_bit) == 0)
                break;
            dma_mask_size++;
            dma_mask_bit = dma_mask_bit << 1;
        }
        SET_U_DMA_BUF_IOCTL_FLAGS_DMA_MASK    (&dev_info, dma_mask_size);
        SET_U_DMA_BUF_IOCTL_FLAGS_DMA_COHERENT(&dev_info, dev_is_dma_coherent(obj->dma_dev));
        SET_U_DMA_BUF_IOCTL_FLAGS_MMAP_MODE   (&dev_info, obj->quirk_mmap_mode);
        dev_info.size = (uint64_t)(obj->size);
        dev_info.addr = (uint64_t)(obj->phys_addr);
        if (copy_to_user(argp, &dev_info, sizeof(dev_info)) != 0)
            ret = -EFAULT;
        else
            ret = 0;
        break;
        }
    case U_DMA_BUF_IOCTL_GET_SYNC: {
        u_dma_buf_ioctl_sync_args sync_args = {0};
        SET_U_DMA_BUF_IOCTL_FLAGS_SYNC_DIR  (&sync_args, obj->sync_direction);
        SET_U_DMA_BUF_IOCTL_FLAGS_SYNC_MODE (&sync_args, obj->sync_mode);
        SET_U_DMA_BUF_IOCTL_FLAGS_SYNC_OWNER(&sync_args, obj->sync_owner);
        sync_args.size = (uint64_t)obj->sync_size;
        sync_args.offset = (uint64_t)obj->sync_offset;
        if (copy_to_user(argp, &sync_args, sizeof(sync_args)) != 0)
            ret = -EFAULT;
        else
            ret = 0;
        break;
        }
    case U_DMA_BUF_IOCTL_SET_SYNC: {
        u_dma_buf_ioctl_sync_args sync_args;
        if (copy_from_user(&sync_args, argp, sizeof(sync_args)) != 0)
            ret = -EFAULT;
        else {
            int sync_command = GET_U_DMA_BUF_IOCTL_FLAGS_SYNC_CMD (&sync_args);
            int sync_direction = GET_U_DMA_BUF_IOCTL_FLAGS_SYNC_CMD (&sync_args);
            int sync_mode = GET_U_DMA_BUF_IOCTL_FLAGS_SYNC_CMD (&sync_args);
            u64 sync_offset = (u64)(sync_args.offset);
            size_t sync_size = (size_t)(sync_args.size);
            switch (sync_direction) {
            case 0: obj->sync_direction = 0; break;
            case 1: obj->sync_direction = 1; break;
            case 2: obj->sync_direction = 2; break;
            default: break;
            }
            if (sync_mode > 0) { obj->sync_mode = sync_mode; }
            if (sync_offset >= 0) { obj->sync_offset = sync_offset; }
            if (sync_size > 0) { obj->sync_size = sync_size; }
            switch (sync_command) {
            case U_DMA_BUF_IOCTL_FLAGS_SYNC_CMD_FOR_CPU:
                obj->sync_for_cpu = 1;
                ret = udmabuf_sync_for_cpu(obj);
                break;
            case U_DMA_BUF_IOCTL_FLAGS_SYNC_CMD_FOR_DEVICE:
                obj->sync_for_device = 1;
                ret = udmabuf_sync_for_device(obj);
                break;
            default:
                ret = 0;
                break;
            }
        }
        break;
        }
    case U_DMA_BUF_IOCTL_SET_SYNC_FOR_CPU: {
        u64 sync_args;
        if (copy_from_user(&sync_args, argp, sizeof(sync_args)) != 0)
            ret = -EFAULT;
        else {
            obj->sync_for_cpu = sync_args;
            ret = udmabuf_sync_for_cpu(obj);
        }
        break;
    }
    case U_DMA_BUF_IOCTL_SET_SYNC_FOR_DEVICE: {
        u64 sync_args;
        if (copy_from_user(&sync_args, argp, sizeof(sync_args)) != 0)
            ret = -EFAULT;
        else {
            obj->sync_for_cpu = sync_args;
            ret = udmabuf_sync_for_device(obj);
        }
        break;
    }
    case U_DMA_BUF_IOCTL_EXPORT: {
        u_dma_buf_ioctl_export_args export_args;
        struct udmabuf_export_entry *export_entry = ERR_PTR(-EINVAL);
        if (copy_to_user(&export_args, argp, sizeof(export_args)) != 0) {
            ret = -EFAULT;
            break;
        } else {
            u64 offset = (u64)(export_args.offset);
            size_t size = (size_t)(export_args.size);
            u32 fd_flags = GET_U_DMA_BUF_IOCTL_FLAGS_EXPORT_FD_FLAGS(&export_args);
            export_entry = udmabuf_export_create_entry(obj, offset, size, fd_flags);
        }
        if (IS_ERR_OR_NULL(export_entry)) {
            ret = PTR_ERR(export_entry);
            export_entry = NULL;
            break;
        }
        export_args.fd = export_entry->fd;
        export_args.addr = export_entry->object_data.phys_addr;
        if (copy_to_user(argp, &export_args, sizeof(export_args)) != 0)
            ret = -EFAULT;
        else
            ret = 0;
        break;
    }
    default:
         ret = -ENOTTY;
    }
    return (long)ret;
}

static const struct file_operations udmabuf_device_file_ops = {
    .owner          = THIS_MODULE,
    .open           = udmabuf_device_file_open,
    .release        = udmabuf_device_file_release,
    .mmap           = udmabuf_device_file_mmap,
    .read           = udmabuf_device_file_read,
    .write          = udmabuf_device_file_write,
    .llseek         = udmabuf_device_file_llseek,
    .unlocked_ioctl = udmabuf_device_file_ioctl,
};



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
    obj->device_number = MKDEV(MAJOR(udmabuf_device_number), minor);

    if (name == NULL)
        obj->sys_dev = device_create(udmabuf_sys_class, parent, obj->device_number,
                            (void *)obj, DEVICE_NAME_FORMAT, MINOR(obj->device_number));
    else
        obj->sys_dev = device_create(udmabuf_sys_class, parent, obj->device_number,
                            (void *)obj, "%s", name);
    if (IS_ERR_OR_NULL(obj->sys_dev)) {
        obj->sys_dev = NULL;
        pr_err(DRIVER_NAME ": device_create failed, return = %ld\n", PTR_ERR(obj->sys_dev));
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


static int udmabuf_object_setup(struct udmabuf_object *obj)
{
    if (!obj)
        return -ENODEV;

    obj->alloc_size = ((obj->size + (((size_t)1 << PAGE_SHIFT) - 1)) >> PAGE_SHIFT) << PAGE_SHIFT;

    obj->virt_addr = dma_alloc_coherent(obj->dma_dev, obj->alloc_size, &obj->phys_addr, GFP_KERNEL);
    if (IS_ERR_OR_NULL(obj->virt_addr)) {
        dev_err(obj->sys_dev, "dma_alloc_coherent(size = %zu) failed. return(%ld\n)", obj->alloc_size, PTR_ERR(obj->virt_addr));
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
        obj->pages = kmalloc_array(obj->pagecount, sizeof(struct page), GFP_KERNEL);
        if (IS_ERR_OR_NULL(obj->pages)) {
            dev_warn(obj->sys_dev, "allocate pages(pagecount=%lu) failed. return(%ld\n)", 
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
                of_reserved_mem_device_release(dev);
        }
    } else {
        ret = -ENODEV;
    }

    return ret;
}

static int udmabuf_platform_device_probe(struct device *dev)
{
    int ret, minor_number;
    u32 u32_value;
    u64 u64_value;
    size_t size;
    struct udmabuf_object *obj = NULL;
    const char *device_name;
    int quirk_mmap_mode;

    if (device_property_read_u64(dev, "size", &u64_value) == 0) {
        size = u64_value;
    } else if ((ret = of_property_read_u64(dev->of_node, "size", &u64_value)) == 0) {
        size = u64_value;
    } else {
        dev_err(dev, "invalid property size. status = %d\n", ret);
        ret = -ENODEV;
        goto failed;
    }

    if (size <= 0) {
        dev_err(dev, "invalid size, size = %ld\n", size);
        ret = -ENODEV;
        goto failed;
    }
    
    if (device_property_read_u32(dev, "minor-number", &u32_value) == 0) {
        minor_number = u32_value;
    } else if ((ret = of_property_read_u64(dev->of_node, "minor-number", &u64_value)) == 0) {
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
            *dev->dma_mask = DMA_BIT_MASK(u32_value);
            dev->coherent_dma_mask = DMA_BIT_MASK(u32_value);
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

    if (of_property_read_u64(dev->of_node, "sync-offset", &u64_value) == 0) {
        if (u64_value >= obj->size) {
            dev_err(dev, "invalid sync-offset property value = %lld\n", u64_value);
            goto failed_with_unlock;
        }
        obj->sync_offset = (int)u64_value;
    }

    if (of_property_read_u64(dev->of_node, "sync-size", &u64_value) == 0) {
        if (obj->sync_offset + u64_value > obj->size) {
            dev_err(dev, "invalid sync-size property value = %lld\n", u64_value);
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

typedef struct {
    char *name;
    int id;
    unsigned int size;
    char *bind_id;
} udmabuf_static_device_param;

extern struct bus_type amba_bustype;
extern struct bus_type pci_bus_type;

static struct bus_type *udmabuf_available_bus_type_list[] = {
    &amba_bustype,
    &pci_bus_type,
    NULL
};

static struct bus_type *udmabuf_find_available_bus_type(char *name, int name_len)
{
    int i;

    if ((name == NULL) || (name_len == 0))
        return NULL;

    for (i = 0; udmabuf_available_bus_type_list[i] != NULL; i++) {
        const char *bus_name;
        if (udmabuf_available_bus_type_list[i] == NULL)
            break;
        bus_name = udmabuf_available_bus_type_list[i]->name;
        if (name_len != strlen(bus_name))
            continue;
        if (strncmp(name, bus_name, name_len) == 0)
            break;
    }
    
    return udmabuf_available_bus_type_list[i];
}

static int udmabuf_static_parse_find(char *bind, struct bus_type **bus_type, char **device_name)
{
    int ret;
    char *next_ptr = strchr(bind, '/');

    if (!next_ptr) {
        *bus_type = &platform_bus_type;
        *device_name = bind;
        ret = 0;
    } else {
        char *name = bind;
        int name_len = next_ptr - bind;
        struct bus_type *found_bus_type = udmabuf_find_available_bus_type(name, name_len);
        if (found_bus_type == NULL)
            ret = -EINVAL;
        else {
            *bus_type = found_bus_type;
            *device_name = next_ptr + 1;
            ret = 0;
        }
    }

    return ret;
}

static struct udmabuf_device_entry *udmabuf_device_list_search(struct device *dev, const char *name, int id)
{
    struct udmabuf_device_entry *entry = NULL;
    struct udmabuf_device_entry *found_entry = NULL;

    mutex_lock(&udmabuf_device_list_sem);
    list_for_each_entry(entry, &udmabuf_device_list, list) {
        bool found_by_dev = true;
        bool found_by_name = true;
        bool found_by_id = true;
        if (dev != NULL) {
            found_by_dev = false;
            if (dev == entry->dev)
                found_by_dev = true;
        }
        if (name != NULL) {
            const char *device_name;
            found_by_name = false;
            if (device_property_read_string(entry->dev, "device-name", &device_name) == 0)
                if (strcmp(name, device_name) == 0)
                    found_by_name = true;
        }
        if (id >= 0) {
            u32 minor_number;
            found_by_id = false;
            if (device_property_read_u32(entry->dev, "minor-number", &minor_number) == 0)
                if (id == minor_number)
                    found_by_id = true;
        }
        if (found_by_dev == true && found_by_name == true && found_by_id == true)
            found_entry = entry;
    }
    mutex_unlock(&udmabuf_device_list_sem);

    return found_entry;
}

static struct udmabuf_device_entry *udmabuf_device_list_create_entry(struct device *dev, struct device *parent,
                                            const char *name, int id, unsigned int size, u64 option,
                                            void (*prep_remove)(struct device *), void (*post_remove)(struct device *))
{
    struct udmabuf_device_entry *exist_entry;
    struct udmabuf_device_entry *entry = NULL;
    struct property_entry  *props;
    struct property_entry props_list[] = {
        PROPERTY_ENTRY_STRING("device-name", name),
        PROPERTY_ENTRY_U64("size", size),
        PROPERTY_ENTRY_U32("minor-number", id),
        PROPERTY_ENTRY_U64("option", option),
        {}
    };
    int ret;

    exist_entry = udmabuf_device_list_search(NULL, name, id);
    if (!IS_ERR_OR_NULL(exist_entry)) {
        pr_err(DRIVER_NAME ": device name(%s) or id(%d) is already exists\n", (name) ? name : "NULL", id);
        ret = -EINVAL;
        goto failed;
    }
    
    entry = kzalloc(sizeof(*entry), GFP_KERNEL);
    if (!entry) {
        ret = PTR_ERR(entry);
        entry = NULL;
        pr_err(DRIVER_NAME ": kzalloc() failed. return=%d", ret);
        goto failed;
    }

    props = (name != NULL) ? &props_list[0] : &props_list[1];
    ret = device_add_properties(dev, props);
    if (ret != 0) {
        pr_err(DRIVER_NAME ": device_add_properties failed. return=%d\n", ret);
        goto failed;
    }

    entry->dev = dev;
    entry->parent = get_device(parent);
    entry->prep_remove = prep_remove;
    entry->post_remove = post_remove;

    mutex_lock(&udmabuf_device_list_sem);
    list_add_tail(&entry->list, &udmabuf_device_list);
    mutex_unlock(&udmabuf_device_list_sem);

    return entry;

failed:
    if (entry != NULL)
        kfree(entry);
    return ERR_PTR(ret);
}

static void udmabuf_child_device_delete(struct device *dev)
{
    char *device_name  = kstrdup(dev_name(dev), GFP_KERNEL);

    udmabuf_object_destroy(dev_get_drvdata(dev));

    if (device_name)
        kfree(device_name);
}

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

static int udmabuf_child_device_create(const char *name, int id, unsigned int size,
                                u64 option, struct device *parent)
{
    const char *device_name;
    struct udmabuf_object *obj;
    struct udmabuf_device_entry *entry = NULL;
    int ret;

    if (size == 0)
        return -EINVAL;

    if ((name == NULL) && (id < 0))
        device_name = "u-dma-buf";
    else
        device_name = name;

    obj = udmabuf_object_create(device_name, parent, id);
    if (IS_ERR_OR_NULL(obj)) {
        ret = PTR_ERR(obj);
        pr_err(DRIVER_NAME ": object create failed. return=%d\n", ret);
        obj = NULL;
        ret = (ret == 0) ? -EINVAL : ret;
        goto failed;
    }
    mutex_lock(&obj->sem);
    obj->size = size;
    udmabuf_set_quirk_mmap_mode(obj, udmabuf_get_option_quirk_mmap_mode(option));
    entry = udmabuf_device_list_create_entry(obj->sys_dev, parent, name, id,
                                        size, option, NULL, udmabuf_child_device_delete);
    if (!entry) {
        ret = PTR_ERR(entry);
        entry = NULL;
        dev_err(obj->sys_dev, "device create entry failed. return=%d\n", ret);
        goto failed_with_unlock;
    }

    ret = udmabuf_object_setup(obj);
    if (ret) {
        dev_err(obj->sys_dev, "object setup failed. return=%d\n", ret);
        goto failed_with_unlock;
    }
    mutex_unlock(&obj->sem);

    return 0;

failed_with_unlock:
    mutex_unlock(&obj->sem);
failed:
    if (entry != NULL)
        udmabuf_device_list_delete_entry(entry);
    if (obj != NULL)
        udmabuf_object_destroy(obj);
    if (entry != NULL)
        put_device(parent);

    return ret;
}

static void udmabuf_platform_device_del(struct device *dev)
{
    platform_device_del(to_platform_device(dev));
}

static void udmabuf_platform_device_put(struct device *dev)
{
    platform_device_put(to_platform_device(dev));
}

static int udmabuf_platform_device_create(const char *name, int id, unsigned int size, u64 option)
{
    struct platform_device *pdev;
    struct udmabuf_device_entry *entry = NULL;
    int ret;
    u64 dma_mask_size = udmabuf_get_option_dma_mask_size(option);

    if (size == 0)
        return -EINVAL;

    pdev = platform_device_alloc(DRIVER_NAME, id);
    if (IS_ERR_OR_NULL(pdev)) {
        ret = PTR_ERR(pdev);
        pdev = NULL;
        pr_err(DRIVER_NAME ": platform_device_alloc(%s,%d) failed. return=%d", DRIVER_NAME, id, ret);
        goto failed;
    }

    if (!pdev->dev.dma_mask)
        pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;

    if (dma_mask_size != 0) {
        pdev->dev.coherent_dma_mask = DMA_BIT_MASK(dma_mask_size);
        *pdev->dev.dma_mask = DMA_BIT_MASK(dma_mask_size);
    } else {
        pdev->dev.coherent_dma_mask = DMA_BIT_MASK(dma_mask_bit);
        *pdev->dev.dma_mask = DMA_BIT_MASK(dma_mask_bit);
    }

    entry = udmabuf_device_list_create_entry(&pdev->dev, NULL, name, id, size, option,
                                    udmabuf_platform_device_del, udmabuf_platform_device_put);

    if (IS_ERR_OR_NULL(entry)) {
        ret = PTR_ERR(entry);
        entry = NULL;
        pr_err(DRIVER_NAME ": device create entry failed. return=%d\n", ret);
        goto failed;
    }

    ret = platform_device_add(pdev);
    if (ret != 0) {
        pr_err(DRIVER_NAME ": platform_device_add failed. return=%d\n", ret);
        goto failed;
    }

    if (dev_get_drvdata(&pdev->dev) == NULL) {
        pr_err(DRIVER_NAME ": object of %s is none.\n", dev_name(&pdev->dev));
        platform_device_del(pdev);
        ret = -ENODEV;
        goto failed;
    }

    return 0;

failed:
    if (entry != NULL)
        udmabuf_device_list_delete_entry(entry);
    if (pdev != NULL)
        platform_device_put(pdev);

    return ret;
}

static int udmabuf_static_device_create(udmabuf_static_device_param *param)
{
    int ret;
    char *name = param->name;
    int id = param->id;
    unsigned int size = param->size;
    char *bind_id = (param->bind_id) ? param->bind_id : bind;
    u64 option = 0;
    struct device *parent = NULL;

    if (bind_id != NULL) {
        struct bus_type *bus_type = NULL;
        char *device_name = NULL;
        ret = udmabuf_static_parse_find(bind_id, &bus_type, &device_name);
        if (ret) {
            pr_err(DRIVER_NAME ": bind error: %s is not support bus\n", bind_id);
            return ret;
        }
        parent = bus_find_device_by_name(bus_type, NULL, device_name);
        if (IS_ERR_OR_NULL(parent)) {
            ret = (parent == NULL) ? -EINVAL : PTR_ERR(parent);
            pr_err(DRIVER_NAME ": bind error: device(%s) not found in bus(%s)\n", device_name, bus_type->name);
            return ret;
        }
    }

    if (parent) {
        ret = udmabuf_child_device_create(name, id, size, option, parent);
        put_device(parent);
    } else {
        ret = udmabuf_platform_device_create(name, id, size, option);
    }

    return 0;
}

#define DEFINE_UDMABUF_STATIC_DEVICE_PARAM(__num)                           \
    static ulong udmabuf ## __num = 0;                                      \
    module_param(udmabuf ## __num, ulong, S_IRUGO);                         \
    MODULE_PARM_DESC(udmabuf ## __num, DRIVER_NAME #__num " buffer size");  \
    static char *udmabuf ## __num ## _bind = NULL;                          \
    module_param(udmabuf ## __num ## _bind, charp, S_IRUGO);                \
    MODULE_PARM_DESC(udmabuf ## __num ## _bind, DRIVER_NAME #__num          \
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

DEFINE_UDMABUF_STATIC_DEVICE_PARAM(0);
DEFINE_UDMABUF_STATIC_DEVICE_PARAM(1);
DEFINE_UDMABUF_STATIC_DEVICE_PARAM(2);
DEFINE_UDMABUF_STATIC_DEVICE_PARAM(3);
DEFINE_UDMABUF_STATIC_DEVICE_PARAM(4);
DEFINE_UDMABUF_STATIC_DEVICE_PARAM(5);
DEFINE_UDMABUF_STATIC_DEVICE_PARAM(6);
DEFINE_UDMABUF_STATIC_DEVICE_PARAM(7);

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

    ret = alloc_chrdev_region(&udmabuf_device_number, 0, DEVICE_MAX_NUM, DRIVER_NAME);
    if (ret != 0) {
        pr_err(DRIVER_NAME ": couldn't allocate device major number, ret = %d\n", ret);
        udmabuf_device_number = 0;
        goto failed;
    }

    udmabuf_sys_class = class_create(THIS_MODULE, DRIVER_NAME);
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
