/*
 *  cma_base.c
 *
 *  brif
 *  	Continuous Memory Allocate driver
 *  
 *  (C) 2024.12.11 <hkdywg@163.com>
 *
 *  This program is free software; you can redistribute it and/r modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 * */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/cma.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/uaccess.h>
#include <linux/dma-mapping.h>
#include <linux/cdev.h>

#define DEVICE_NAME 	"cma_demo"

#define CMA_REGION_NUM 		(64)
#define CMA_MEM_ALLOCATE 	_IOW('m', 1, unsigned int)
#define CMA_MEM_RELEASE 	_IOW('m', 2, unsigned int)

/* CMA region information */
struct cma_demo_info {
	unsigned long virt;
	unsigned long phys;
	unsigned long offset;
	unsigned long length;
};


/* CMA Memory region */
struct cma_demo_region {
	struct cma_demo_info info;
	dma_addr_t dma_handle;
	struct list_head list;
};

/* CMA manager information */
struct cma_demo_manager {
	struct miscdevice misc;
	struct mutex lock;
	struct list_head head;
};

/* CMA memory dvice */
static struct cma_demo_manager *cma_manager;


static long cma_demo_ioctl(struct file *filp, unsigned int cmd,
						   unsigned long arg)
{
	struct cma_demo_region *region;
	struct cma_demo_info info;
	void *cma_addr;
	bool found = false;
	int ret;

	switch(cmd) {
	case CMA_MEM_ALLOCATE:
		/* lock */
		mutex_lock(&cma_manager->lock);
		/* get information from userland */
		copy_from_user(&info, (void __user *)arg, sizeof(struct cma_demo_info));

		/* allocate new region */
		region = kzalloc(sizeof(struct cma_demo_region), GFP_KERNEL);
		if(!region) {
			printk("ALLOCATE: no memory\n");
			ret = -ENOMEM;
			goto err_alloc;
		}

		/* allocate memory from CMA */
		cma_addr = dma_alloc_coherent(cma_manager->misc.this_device, info.length, &region->dma_handle, GFP_KERNEL | __GFP_COMP);
		if(!cma_addr) {
			printk("ALLOCATE: DMA allocate error\n");
			ret = -ENOMEM;
			goto err_dma;
		}

		/* Insert region into cma manager */
		info.virt = (dma_addr_t)cma_addr;
		info.phys = (dma_addr_t)virt_to_phys(cma_addr);
		region->info.virt = info.virt;
		region->info.phys = info.phys;
		region->info.length = info.length;
		list_add(&region->list, &cma_manager->head);

		/* export to userland */
		copy_to_user((void __user *)arg, &info, sizeof(info));
		/* unlock */
		mutex_unlock(&cma_manager->lock);
		break;
	case CMA_MEM_RELEASE:
		mutex_lock(&cma_manager->lock);
		copy_from_user(&info, (void __user *)arg, sizeof(info));

		/* search region in cma manager list */
		list_for_each_entry(region, &cma_manager->head, list) {
			if(region->info.phys == info.phys &&
			   region->info.length == info.length) {
				found = true;
				break;
			}
		}
		if(!found) {
			printk("RELEASE: Can't find specical region\n");
			ret = -EINVAL;
			goto err_alloc;
		}
		/* free contiguous memory */
		cma_addr = phys_to_virt(info.phys);
		dma_free_coherent(cma_manager->misc.this_device, info.length, cma_addr, region->dma_handle);
		list_del(&region->list);
		kfree(region);
		mutex_unlock(&cma_manager->lock);
		break;
	default:
		printk("CMA not support command\n");
		return -EFAULT;
	}

	return 0;

err_dma:
	kfree(region);
err_alloc:
	return ret;
}

static int cma_demo_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long start = vma->vm_start;
	unsigned long size = vma->vm_end - vma->vm_start;
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
	unsigned long page;

	/* offset is physical address */
	page = offset >> PAGE_SHIFT;

	/* remap */
	if(remap_pfn_range(vma, start, page, size, PAGE_SHARED)) {
		printk("REMAP: failed\n");
		return -EAGAIN;
	}

	vma->vm_flags &= ~VM_IO;
	vma->vm_flags |= (VM_DONTEXPAND | VM_DONTDUMP);

	return 0;

}

static struct file_operations cma_demo_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = cma_demo_ioctl,
	.mmap = cma_demo_mmap,
};

static int __init cma_demo_init(void)
{
	int ret;

	cma_manager = kzalloc(sizeof(struct cma_demo_manager), GFP_KERNEL);
	if(!cma_manager) {
		printk("Allocate cma demo manager memory failed\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	/* mutex lock init */
	mutex_init(&cma_manager->lock);

	/* misc: initialize */
	cma_manager->misc.name = DEVICE_NAME;
	cma_manager->misc.minor = MISC_DYNAMIC_MINOR;
	cma_manager->misc.fops = &cma_demo_fops;

	/* manager list initilize */
	INIT_LIST_HEAD(&cma_manager->head);

	/* register misc device */
	misc_register(&cma_manager->misc);

	return 0;

err_alloc:
	return ret;
}

static void __exit cma_demo_exit(void)
{
	struct cma_demo_region *region;

	mutex_lock(&cma_manager->lock);
	list_for_each_entry(region, &cma_manager->head, list) 
		kfree(region);
	mutex_unlock(&cma_manager->lock);

	misc_deregister(&cma_manager->misc);
	kfree(cma_manager);
	cma_manager = NULL;
}

module_init(cma_demo_init);
module_exit(cma_demo_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("yinwg <hkdywg@163.com>");
MODULE_DESCRIPTION("dma driver");

