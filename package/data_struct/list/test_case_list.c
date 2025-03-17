/*
 *  test_case_list.c
 *
 *  brif
 *  	test case of list operation interface
 *  
 *  (C) 2025.03.14 <hkdywg@163.com>
 *
 *  This program is free software; you can redistribute it and/r modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 * */
#include "list.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>

struct task_info
{
	char *name;
	unsigned int init_tick;
	unsigned int current_prio;

	list_t tlist;
};

struct task_info user_task[] = {
	{
		.name = "task_schedule",
		.init_tick = 10,
		.current_prio = 20,
	},
	{
		.name = "memory_manager",
		.init_tick = 10,
		.current_prio = 10,
	},
	{
		.name = "file_system_manager",
		.init_tick = 5,
		.current_prio = 30,
	},
	{
		.name = "driver_framework",
		.init_tick = 10,
		.current_prio = 5,
	},
};

list_t *entry;
/* Initialize task list head */
LIST_HEAD(__task_list);

void test_list_add()
{
	/* test case of list add  */	
	printf("================ sart test case of list add operation ================ \n");
	for(int i = 0; i < sizeof(user_task)/sizeof(struct task_info); i++)
		list_add_tail(&__task_list, &user_task[i].tlist);

	list_for_each(entry, &__task_list) {
		struct task_info *task = list_entry(entry, struct task_info, tlist); 	
		printf("task name: %s\n", task->name);
	}
	printf("================ end test case of list add operation ================ \n\n");
}

void test_list_delete()
{
	printf("================ sart test case of list delete operation ================ \n");

	for(int i = 0; i < sizeof(user_task)/sizeof(struct task_info); i++)
		list_add_tail(&__task_list, &user_task[i].tlist);
	/* test case of delete */
	list_for_each(entry, &__task_list) {
		struct task_info *task = list_entry(entry, struct task_info, tlist); 	
		if(strcmp(task->name, "file_system_manager") == 0) {
			list_del(entry);
			break;
		}
	}
	list_for_each(entry, &__task_list) {
		struct task_info *task = list_entry(entry, struct task_info, tlist); 	
		printf("task name: %s\n", task->name);
	}
	printf("================ end test case of list delete operation ================ \n\n");
}

void test_list_replace()
{
	struct task_info new_task = {
		.name = "interrupt_manager",
		.init_tick = 10,
		.current_prio = 10,
	};
	printf("================ sart test case of list replace operation ================ \n");
	list_t *old = NULL;
	for(int i = 0; i < sizeof(user_task)/sizeof(struct task_info); i++)
		list_add_tail(&__task_list, &user_task[i].tlist);
	/* test case of delete */
	list_for_each(entry, &__task_list) {
		struct task_info *task = list_entry(entry, struct task_info, tlist); 	
		if(strcmp(task->name, "file_system_manager") == 0) {
			old = entry;
			list_replace(old, &new_task.tlist);
			break;
		}
	}
	list_for_each(entry, &__task_list) {
		struct task_info *task = list_entry(entry, struct task_info, tlist); 	
		printf("task name: %s\n", task->name);
	}
	printf("================ end test case of list replace operation ================ \n\n");
}

void test_list_rotate()
{
	printf("================ sart test case of list rotate operation ================ \n");

	for(int i = 0; i < sizeof(user_task)/sizeof(struct task_info); i++)
		list_add_tail(&__task_list, &user_task[i].tlist);
	/* test case of rotate */
	list_rotate_left(&__task_list);
	list_for_each(entry, &__task_list) {
		struct task_info *task = list_entry(entry, struct task_info, tlist); 	
		printf("task name: %s\n", task->name);
	}
	/* test case of rotate */
	list_rotate_right(&__task_list);
	list_for_each(entry, &__task_list) {
		struct task_info *task = list_entry(entry, struct task_info, tlist); 	
		printf("task name: %s\n", task->name);
	}
	printf("================ end test case of list rotate operation ================ \n\n");
}

void test_list_cut()
{
	printf("================ sart test case of list cut operation ================ \n");

	LIST_HEAD(new_list);
	for(int i = 0; i < sizeof(user_task)/sizeof(struct task_info); i++)
		list_add_tail(&__task_list, &user_task[i].tlist);

	list_for_each(entry, &__task_list) {
		struct task_info *task = list_entry(entry, struct task_info, tlist); 	
		printf("origin task name: %s\n", task->name);
	}
	printf("---------------------------------\n");
	/* test case of cut */
	list_for_each(entry, &__task_list) {
		struct task_info *task = list_entry(entry, struct task_info, tlist); 	
		if(strcmp(task->name, "file_system_manager") == 0) {
			list_cut_befor(&new_list, &__task_list, entry);
			break;
		}
	}
	list_for_each(entry, &__task_list) {
		struct task_info *task = list_entry(entry, struct task_info, tlist); 	
		printf("new task list_1 name: %s\n", task->name);
	}
	printf("---------------------------------\n");
	list_for_each(entry, &new_list) {
		struct task_info *task = list_entry(entry, struct task_info, tlist); 	
		printf("new task list_2 name: %s\n", task->name);
	}
	printf("---------------------------------\n");
	list_splice(&new_list, &__task_list);
	list_for_each(entry, &__task_list) {
		struct task_info *task = list_entry(entry, struct task_info, tlist); 	
		printf("splice task list name: %s\n", task->name);
	}

	printf("================ end test case of list cut operation ================ \n\n");
}

int main(int argc, char *argv[])
{

	int opt;
    while ((opt = getopt(argc, argv, "adrlc")) != -1) {
        switch (opt) {
            case 'a':
				test_list_add();
                break;
            case 'd':
				test_list_delete();
                break;
            case 'r':
				test_list_replace();
                break;
            case 'l':
				test_list_rotate();
                break;
            case 'c':
				test_list_cut();
                break;
            default:
                printf("未知选项\n");
        }
    }

	return 0;
}



