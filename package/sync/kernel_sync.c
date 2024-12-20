/*
 *  kernel_sync.c
 *
 *  brif
 *  	linux kernel Synchronization mechanism
 *  
 *  (C) 2024.12.20 <hkdywg@163.com>
 *
 *  This program is free software; you can redistribute it and/r modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 * */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/delay.h>

enum sync_mech {
	USE_DEFAULT 		= 0x00,
	USE_ATOMIC_MECH 	= 0x01,
	USE_MUTEX_MECH 		= 0x02,
	USE_SPINLOCK_MECH 	= 0x04,
	USE_RW_LOCK_MECH 	= 0x08,
};

struct thread_sync_muster {
	enum sync_mech use_mech;
	atomic_t *atomic_var;
	struct mutex *mutex;
};

static uint32_t no_atomic_var = 0;

static int kernel_test_thread(void *arg)
{
	struct thread_sync_muster *muster = (struct thread_sync_muster *)arg; 
	uint32_t i = 0;

	while(1) {
		switch(muster->use_mech)
		{
			case USE_ATOMIC_MECH: 
				for(i = 0; i < 10000000; i++) {
					atomic_add(2, muster->atomic_var);
					atomic_dec(muster->atomic_var);
					no_atomic_var+=2;
					--no_atomic_var;
				}
				return 0;
				break;
			case USE_DEFAULT:
				msleep(1000);
				printk("atomic variable is : %d\n", atomic_read(muster->atomic_var));
				printk("not atomic variable is : %d\n", no_atomic_var);
				break;
		}
	}
	return 0;
}


static int __init kernel_sync_init(void)
{
	static atomic_t atomic_var = ATOMIC_INIT(0);
	uint8_t i;
	char thread_name[32];
	struct task_struct *tasks[64];

	static struct thread_sync_muster thread_arg[] = {
		{ USE_ATOMIC_MECH,  &atomic_var, NULL },
		{ USE_ATOMIC_MECH,  &atomic_var, NULL},
		{ USE_DEFAULT,  &atomic_var, NULL},
	};

	for(i = 0; i < sizeof(thread_arg)/sizeof(struct thread_sync_muster); i++) {
		scnprintf(thread_name, sizeof(thread_name), "%s%d", "kernel_thread_", i);
		tasks[i] = kthread_create(kernel_test_thread, &thread_arg[i], thread_name);
	}

	for(i = 0; i < sizeof(thread_arg)/sizeof(struct thread_sync_muster); i++) {
		wake_up_process(tasks[i]);
	}

	return 0;
}

static void __exit kernel_sync_exit(void)
{

}

module_init(kernel_sync_init);
module_exit(kernel_sync_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hkdywg <hkdywg@163.com>");
MODULE_DESCRIPTION("LinuxLab example device driver");

