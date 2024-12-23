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
#include <linux/compiler.h>
#include <linux/rwlock_types.h>

/* Memory access
 *
 *
 *      +----------+
 *      |          |
 *      | Register |                                         +--------+
 *      |          |                                         |        |
 *      +----------+                                         |        |
 *            A                                              |        |
 *            |                                              |        |
 * +-----+    |      +----------+        +----------+        |        |
 * |     |<---o      |          |        |          |        |        |
 * | CPU |<--------->| L1 Cache |<------>| L2 Cache |<------>| Memory |
 * |     |<---o      |          |        |          |        |        |
 * +-----+    |      +----------+        +----------+        |        |
 *            |                                              |        |
 *            o--------------------------------------------->|        |
 *                         volatile/atomic                   |        |
 *                                                           |        |
 *                                                           +--------+
 */

enum sync_mech {
	USE_DEFAULT 		= 0x00,
	USE_ATOMIC_MECH 	= 0x01,
	USE_MUTEX_MECH 		= 0x02,
	USE_SPINLOCK_MECH 	= 0x04,
	USE_RWLOCK_MECH 	= 0x08,
};

struct thread_sync_muster {
	enum sync_mech use_mech;
	atomic_t *atomic_var;
	struct mutex *mutex;
	rwlock_t *rw_lock;
	uint8_t sub_id;
};

static uint32_t no_atomic_var = 0;
/* Declar and init rwlock */
DEFINE_RWLOCK(rw_lock);
/* Declar and init mutex */
DEFINE_MUTEX(mutex);

static int kernel_test_thread(void *arg)
{
	struct thread_sync_muster *muster = (struct thread_sync_muster *)arg; 
	uint32_t rwlock_counter = 0xA5;
	uint32_t mutex_counter = 0x6B;
	volatile char ch_a = 'A';
	char ch_b = 'B';
	char cb;
	uint32_t i = 0;

	while(1) {
		switch(muster->use_mech)
		{
			case USE_ATOMIC_MECH: 
				/* Read from memory not cache nor register */
				__read_once_size(&ch_a, &cb, sizeof(ch_a));
				printk("read cha is %c\n", cb);
				__write_once_size(&ch_a, &ch_b, sizeof(ch_b));
				cb = READ_ONCE(ch_a);
				printk("read cha after write_once_size is %c\n", cb);
				for(i = 0; i < 10000000; i++) {
					atomic_add(2, muster->atomic_var);
					atomic_dec(muster->atomic_var);
					no_atomic_var+=2;
					--no_atomic_var;
				}
				return 0;
				break;
			case USE_MUTEX_MECH:
				mutex_lock(muster->mutex);
				if(muster->sub_id == 0x01)
					++mutex_counter;
				else
					--mutex_counter;
				mutex_unlock(muster->mutex);
				break;
			case USE_RWLOCK_MECH:
				write_lock(muster->rw_lock);
				if(muster->sub_id == 0x01)
					++rwlock_counter;
				else
					--rwlock_counter;
				write_unlock(muster->rw_lock);
				msleep(100);
				break;
			case USE_DEFAULT:
				msleep(1000);
				/* atomic */
				printk("atomic variable is : %d\n", atomic_read(muster->atomic_var));
				printk("not atomic variable is : %d\n", no_atomic_var);
				/* mutex */
				mutex_lock(muster->mutex);
				printk("mutex counter is : 0x%x\n", mutex_counter);
				mutex_unlock(muster->mutex);
				/* read lock */
				read_lock(muster->rw_lock);
				printk("rwlock couner is : 0x%x\n", rwlock_counter);
				read_unlock(muster->rw_lock);
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
		{ USE_ATOMIC_MECH,  &atomic_var, NULL, NULL },
		{ USE_ATOMIC_MECH,  &atomic_var, NULL, NULL},
		{ USE_MUTEX_MECH, NULL, &mutex, NULL, 0x1},
		{ USE_MUTEX_MECH, NULL, &mutex, NULL, 0x2},
		{ USE_RWLOCK_MECH, NULL, NULL, &rw_lock, 0x1 },
		{ USE_RWLOCK_MECH, NULL, NULL, &rw_lock, 0x2 },
		{ USE_DEFAULT,  &atomic_var, &mutex, &rw_lock},
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

