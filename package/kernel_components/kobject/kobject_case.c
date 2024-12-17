/*
 *  kobject_case.c
 *
 *  brif
 *  	kobject use case
 *  
 *  (C) 2024.12.17 <hkdywg@163.com>
 *
 *  This program is free software; you can redistribute it and/r modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 * */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/rbtree.h>
#include <linux/rbtree_augmented.h>

static struct kobject *kobj;
#define rb_to_kn(X)	rb_entry((X), struct kernfs_node, rb)

static void print_kobject_info(struct kobject *kobj)
{
	if(kobj) {
		printk("kobject name: %s\n", kobj->name);	
		printk("kobject kref: %d\n", kobj->kref);	
		printk("kobject sys diretory: %s\n", kobj->sd->name);
	}
}


static int __init kobject_case_init(void)
{
	struct kernfs_node *kn, *pos;
	struct rb_root *rbroot;
	struct rb_node *np;

	kobj = kobject_create_and_add("LinuxLab", NULL);
	if(!kobj) {
		return -ENOMEM;
	}

	print_kobject_info(kobj);
	print_kobject_info(kobj->parent);
	kn = kobj->sd;

	/* parent rbtree */
	rbroot = &kn->parent->dir.children;
	/* traverser all knode on rbtree */
	for(np = rb_first_postorder(rbroot); np; 
		np = rb_next_postorder(np)) {
		pos = rb_to_kn(np);
		printk("name: %s\n", pos->name);
	}

	return 0;
}

static void __exit kobject_case_exit(void)
{
	kobject_put(kobj);
}

module_init(kobject_case_init);
module_exit(kobject_case_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hkdywg <hkdywg@163.com>");
MODULE_DESCRIPTION("LinuxLab example device driver");

