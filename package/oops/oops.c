/*
 *  tinyfs.c
 *  
 *  (C) 2024.12.04 <hkdywg@163.com>
 *
 *  This program is free software; you can redistribute it and/r modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 * */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/sched/signal.h>
#include <linux/kthread.h>



static int soft_lockup_fn(void *arg)
{
	while(!kthread_should_stop()) {
		while(true)
			cpu_relax();	// WAIT
		printk("task thread runing.\n");
	}

	return 0;
}


static int __init sample_oops_init(void)
{
	static struct task_struct *kthread;

	kthread = kthread_run(soft_lockup_fn, NULL, "soft-lockup");
	if(!kthread)
		return -EINVAL;

	return 0;
}

static void __exit sample_oops_exit(void)
{
}


module_init(sample_oops_init);
module_exit(sample_oops_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("yinwg <hkdywg@163.com>");
MODULE_DESCRIPTION("kernel OOPS sample");
