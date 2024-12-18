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
#include <linux/slab.h>

struct demo_dev {
	struct kobject kobj;
	int dev_size;
};

struct demo_sysfs_entry {
	struct attribute attr;
	ssize_t (*show)(struct demo_dev *, char *);
	ssize_t (*store)(struct demo_dev *, char *, size_t);
};

#define rb_to_kn(X)	rb_entry((X), struct kernfs_node, rb)

static struct kobject *kobj;
static struct demo_dev *p_dev;

static void demo_release(struct kobject *kobj)
{
	struct demo_dev *dev = container_of(kobj, struct demo_dev, kobj);

	kfree(dev);
}

static ssize_t demo_attr_show(struct kobject *kobj,
				struct attribute *attr, char *page)
{
	return 0;
}

static ssize_t demo_attr_store(struct kobject *kobj,
				struct attribute *attr, const char *page, size_t len)
{
	return 0;
}

static const struct sysfs_ops demo_sysfs_ops = {
	.show = demo_attr_show,
	.store = demo_attr_store,
};

static ssize_t size_show(struct demo_dev *dev, char *page)
{
	return sprintf(page, "%d\n", (int)dev->dev_size);
}

static ssize_t size_store(struct demo_dev *dev, char *buf, size_t len)
{
	sscanf(buf, "%d", &dev->dev_size);
	return len;
}


static struct demo_sysfs_entry demo_size =
__ATTR(LinuxLab, S_IRUGO | S_IWUSR, size_show, size_store);

static struct attribute *demo_default_attrs[] = {
	&demo_size.attr,
	NULL,
};

/* kset */
static struct kobj_type demo_type = {
	.release = demo_release,
	.sysfs_ops = &demo_sysfs_ops,
	.default_attrs = demo_default_attrs,
};

static int kobject_init_private(void)
{
	int ret;

	p_dev = kzalloc(sizeof(*p_dev), GFP_KERNEL);
	if(!p_dev) {
		printk("Error: kzalloc\n");
		ret = -ENOMEM;
		return ret;
	}

	kobject_init(&p_dev->kobj, &demo_type);

	return 0;
}


static void print_kobject_info(struct kobject *kobj)
{
	if(kobj) {
		printk("kobject name: %s\n", kobj->name);	
		printk("kobject kref: %d\n", kref_read(&kobj->kref));	
		printk("kobject sys diretory: %s\n", kobj->sd->name);
	}
}


static int __init kobject_case_init(void)
{
	struct kernfs_node *kn, *pos;
	struct rb_root *rbroot;
	struct rb_node *np;
	char *devpath = NULL;
	int ret = 0;

	kobj = kobject_create_and_add("LinuxLab", NULL);
	if(!kobj) {
		return -ENOMEM;
	}

	print_kobject_info(kobj);

	printk("befor get option, kobject kref: %d\n", kref_read(&kobj->kref));	
	kobj = kobject_get(kobj);
	printk("after get option, kobject kref: %d\n", kref_read(&kobj->kref));	
	kobject_put(kobj);
	printk("after put option, kobject kref: %d\n", kref_read(&kobj->kref));	

	/* get path */
	devpath = kobject_get_path(kobj, GFP_KERNEL);
	if(!devpath) {
		printk("Error: kobject get path failed\n");
		ret = -ENOMEM;
		goto err_path;
	}
	printk("LinuxLab path: /sys/%s\n", devpath);

	kn = kobj->sd;

	/* parent rbtree */
	rbroot = &kn->parent->dir.children;
	/* traverser all knode on rbtree */
	for(np = rb_first_postorder(rbroot); np; 
		np = rb_next_postorder(np)) {
		pos = rb_to_kn(np);
		printk("name: %s\n", pos->name);
	}

	kobject_init_private();

	return 0;

err_path:
	kobject_put(kobj);
	return ret;
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

