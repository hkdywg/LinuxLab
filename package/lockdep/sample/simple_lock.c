#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/thread_info.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/delay.h>

static DEFINE_MUTEX(lock_a); // 锁 A
static DEFINE_MUTEX(lock_b); // 锁 B

// 线程 1：首先锁定 lock_a，然后锁定 lock_b
static int thread1_fn(void *data)
{
    pr_info("Thread 1: Trying to lock lock_a...\n");
    mutex_lock(&lock_a);
    pr_info("Thread 1: Acquired lock_a\n");

    // 模拟某些操作
    msleep(100);

    pr_info("Thread 1: Trying to lock lock_b...\n");
    mutex_lock(&lock_b); // 在获取 lock_a 后尝试获取 lock_b，可能导致死锁

    pr_info("Thread 1: Acquired lock_b\n");
    mutex_unlock(&lock_b);
    mutex_unlock(&lock_a);
    
    return 0;
}

// 线程 2：首先锁定 lock_b，然后锁定 lock_a
static int thread2_fn(void *data)
{
    pr_info("Thread 2: Trying to lock lock_b...\n");
    mutex_lock(&lock_b);
    pr_info("Thread 2: Acquired lock_b\n");

    // 模拟某些操作
    msleep(100);

    pr_info("Thread 2: Trying to lock lock_a...\n");
    mutex_lock(&lock_a); // 在获取 lock_b 后尝试获取 lock_a，导致死锁

    pr_info("Thread 2: Acquired lock_a\n");
    mutex_unlock(&lock_a);
    mutex_unlock(&lock_b);

    return 0;
}

static struct task_struct *thread1;
static struct task_struct *thread2;

static int __init deadlock_init(void)
{
    pr_info("Deadlock example: Initializing\n");

    // 创建线程 1
    thread1 = kthread_run(thread1_fn, NULL, "thread1");
    if (IS_ERR(thread1)) {
        pr_err("Failed to create thread 1\n");
        return PTR_ERR(thread1);
    }

    // 创建线程 2
    thread2 = kthread_run(thread2_fn, NULL, "thread2");
    if (IS_ERR(thread2)) {
        pr_err("Failed to create thread 2\n");
        return PTR_ERR(thread2);
    }

    return 0;
}

static void __exit deadlock_exit(void)
{
    pr_info("Deadlock example: Exiting\n");
    // Clean up threads if they are still running
    if (!IS_ERR(thread1))
        kthread_stop(thread1);
    if (!IS_ERR(thread2))
        kthread_stop(thread2);
}

module_init(deadlock_init);
module_exit(deadlock_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hkdywg");
MODULE_DESCRIPTION("AB-BA Deadlock Example");

