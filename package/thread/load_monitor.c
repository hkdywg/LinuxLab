/*
 *  load_monitor.c
 *
 *  brif
 *  	print cpu load
 *  
 *  (C) 2024.12.19 <hkdywg@163.com>
 *
 *  This program is free software; you can redistribute it and/r modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 * */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/sched/task.h>
#include <linux/sched/signal.h>
#include <linux/kallsyms.h>
#include <linux/tracepoint.h>
#include <linux/stacktrace.h>

#define FSHIFT 			11 				/* nr of bits of precision */
#define FIXED_1 		(1<<FSHIFT)		/* 1.0 as fixed-point */
#define LOAD_INT(x)		((x) >> FSHIFT)
#define LOAD_FRAC(x)	LOAD_INT(((x) & (FIXED_1 - 1)) * 100)

#define BACKTRACE_DEPTH 	20

static unsigned long *ptr_avenrun;
static struct hrtimer timer;

static void print_all_task_stack(void)
{
	struct task_struct *g, *p;
	unsigned long backtrace[BACKTRACE_DEPTH];
	struct stack_trace trace;

	printk("\tLoad: %lu.%02lu, %lu.%02lu, %lu.%02lu\n",
		   LOAD_INT(ptr_avenrun[0]), LOAD_FRAC(ptr_avenrun[0]),
		   LOAD_INT(ptr_avenrun[1]), LOAD_FRAC(ptr_avenrun[1]),
		   LOAD_INT(ptr_avenrun[2]), LOAD_FRAC(ptr_avenrun[2]));

	rcu_read_lock();

	printk("dump runing task.\n");
	do_each_thread(g, p) {
		if(p->state == TASK_RUNNING) {
			printk("runing task, comm: %s, pid %d\n", p->comm, p->pid);
			memset(&trace, 0, sizeof(trace));
			memset(backtrace, 0, BACKTRACE_DEPTH * sizeof(unsigned long));
			trace.max_entries = BACKTRACE_DEPTH;
			trace.entries = backtrace;
			save_stack_trace_tsk(p, &trace);
			stack_trace_print(backtrace, trace.nr_entries, 0);
		}
	} while_each_thread(g, p);

	printk("dump uninterrupted task.\n");
	do_each_thread(g, p) {
		if(p->state == TASK_UNINTERRUPTIBLE) {
			printk("runing task, comm: %s, pid %d\n", p->comm, p->pid);
			memset(&trace, 0, sizeof(trace));
			memset(backtrace, 0, BACKTRACE_DEPTH * sizeof(unsigned long));
			trace.max_entries = BACKTRACE_DEPTH;
			trace.entries = backtrace;
			save_stack_trace_tsk(p, &trace);
			stack_trace_print(backtrace, trace.nr_entries, 0);
		}
	} while_each_thread(g, p);

	rcu_read_unlock();
}

static void check_load(void)
{
	static ktime_t last;
	u64 ms;
	int load = LOAD_INT(ptr_avenrun[0]);	// recent 1 minute load value

	ms = ktime_to_ms(ktime_sub(ktime_get(), last));
	if(ms < 5 * 1000)
		return;

	last = ktime_get();
	print_all_task_stack();
}

static enum hrtimer_restart monitor_handler(struct hrtimer *hrtimer)
{
	enum hrtimer_restart ret = HRTIMER_RESTART;

	check_load();

	hrtimer_forward_now(hrtimer, ms_to_ktime(10));

	return ret;
}

static void start_timer(void)
{
	hrtimer_init(&timer, CLOCK_MONOTONIC, HRTIMER_MODE_PINNED);
	timer.function = monitor_handler;
	hrtimer_start_range_ns(&timer, ms_to_ktime(10), 0, HRTIMER_MODE_REL_PINNED);
}

static int __init load_monitor_init(void)
{
	ptr_avenrun = (void *)kallsyms_lookup_name("avenrun");
	if(!ptr_avenrun) {
		printk("Error: kallsyms_lookup_name failed\n");
		return -EINVAL;
	}

	start_timer();

	return 0;
}

static void __exit load_monitor_exit(void)
{

}


module_init(load_monitor_init);
module_exit(load_monitor_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hkdywg <hkdywg@163.com>");
MODULE_DESCRIPTION("LinuxLab example device driver");

