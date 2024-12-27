/*
 *  buddy_alloctor.c
 *
 *  brif
 *  	buddy alloctor test 
 *  
 *  (C) 2024.12.23 <hkdywg@163.com>
 *
 *  This program is free software; you can redistribute it and/r modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 * */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mm.h>

struct gfp_info {
	gfp_t type;
	char *name;
};

static int __init buddy_alloctor_init(void)
{
	int i = 0; 
	struct gfp_info alloc_info[] = { {GFP_KERNEL, "gfp_kernel"},
									{GFP_DMA, "gfp_dma"},
									{__GFP_HIGHMEM, "gfp_highmem"},
									{GFP_USER, "gfp_user"},
							};
	struct page *page;
	void *mem;

	for(i = 0; i < sizeof(alloc_info)/sizeof(struct gfp_info); i++) {
		page = alloc_pages(alloc_info[i].type, 6);
		if(!page) {
			printk("Error: not enough memory\n");
			return -ENOMEM; 
		}
		mem = page_to_virt(page);
		sprintf((char *)mem, "test buddy alloctor memory");
		printk("buddy allocator alloc type: %s, address: %#lx\n", alloc_info[i].name, (unsigned long)mem);
		printk("type: %x\n", alloc_info[i].type);
		__free_pages(page, 6);
	}

	return 0;
}

static void __exit buddy_alloctor_exit(void)
{

}


module_init(buddy_alloctor_init);
module_exit(buddy_alloctor_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hkdywg <hkdywg@163.com>");
MODULE_DESCRIPTION("LinuxLab example device driver");

