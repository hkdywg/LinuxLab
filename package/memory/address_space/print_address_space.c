/*
 *  print_address_space.c
 *
 *  brif
 *  	print thread address space
 *  
 *  (C) 2024.12.23 <hkdywg@163.com>
 *
 *  This program is free software; you can redistribute it and/r modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 * */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>

/* structure */
struct struct_type {
	char *name;
	int number;
};

/* union */
union union_type {
	unsigned long node;
	int count;
};

/* enum */
enum ENUM_TYPE {
	ENUM_ZERO,
	ENUM_ONE
};

#define GUARD_PAGE 	(4096)

/* Global none init data*/
int global_none_init_variable;
int *global_none_init_pointer;
int global_none_init_array[10];
enum ENUM_TYPE global_none_init_enum;
struct struct_type global_none_init_struct;
union union_type global_none_init_union;

/* static Local init data*/
static int global_init_variable = 0x1234;
static int *global_init_pointer = &global_init_variable;
static int global_init_array[10] = {0xa5};
static enum ENUM_TYPE global_init_enum = ENUM_ZERO;
static struct struct_type global_init_struct = {"newversion", 9};
static union union_type global_init_union = {.node = 10};


int main(int argc, char *argv[])
{
	extern char __executable_start[];
	extern char edata[];
	extern char etext[];
	extern char end[];
	intptr_t sp, bp;

	/* Bottom for stack */
	__asm__ volatile ("mov %0, x29" : "=r" (bp));

	/* Local none init data*/
	int local_none_init_variable;
	int *local_none_init_pointer;
	int local_none_init_array[10];
	enum ENUM_TYPE local_none_init_enum;
	struct struct_type local_none_init_struct;
	union union_type local_none_init_union;
	/* Local init data*/
	int local_init_variable = 0x1234;
	int *local_init_pointer = &local_init_variable;
	int local_init_array[10] = {0xa5};
	enum ENUM_TYPE local_init_enum = ENUM_ZERO;
	struct struct_type local_init_struct = {"newversion", 9};
	union union_type local_init_union = {.node = 10};
	/* static Local none init data*/
	static int static_local_none_init_variable;
	static int *static_local_none_init_pointer;
	static int static_local_none_init_array[10];
	static enum ENUM_TYPE static_local_none_init_enum;
	static struct struct_type static_local_none_init_struct;
	static union union_type static_local_none_init_union;
	/* static Local init data*/
	static int static_local_init_variable = 0x1234;
	static int *static_local_init_pointer = &static_local_init_variable;
	static int static_local_init_array[10] = {0xa5};
	static enum ENUM_TYPE static_local_init_enum = ENUM_ZERO;
	static struct struct_type static_local_init_struct = {"newversion", 9};
	static union union_type static_local_init_union = {.node = 10};

	/* Top for stack */
	__asm__ volatile ("mov %0, sp" : "=r" (sp));

	printf("***************************************************************\n");                                                                                       
	printf("+---+-------+-------+------+--+------+-+------+--------------+\n");                                                                                        
	printf("|   |       |       |      |  |      | |      |              |\n");                                                                                        
	printf("|   | .text | .data | .bss |  | Heap | | Mmap |        Stack |\n");                                                                                        
	printf("|   |       |       |      |  |      | |      |              |\n");                                                                                        
	printf("+---+-------+-------+------+--+------+-+------+--------------+\n");                                                                                        
	printf("0                                                            TASK_SIZE\n");     
	printf("Executable start:%#016lx\n", (unsigned long)__executable_start);
	printf("Code range:		%#016lx -- %#016lx\n", (unsigned long)__executable_start, (unsigned long)etext);
	printf("Data range:		  %#016lx -- %#016lx\n", (unsigned long)etext, (unsigned long)edata);
	printf("Bss range:		  %#016lx -- %#016lx\n", (unsigned long)edata, (unsigned long)end);
	printf("Heap:			  %#016lx -- %#016lx\n", (unsigned long)end, (unsigned long)sbrk(0));
	printf("Mmap:			  %#016lx -- %#016lx\n", (unsigned long)sbrk(0) + GUARD_PAGE, (unsigned long)sp - GUARD_PAGE);
	printf("Stack:			  %#016lx -- %#016lx\n", (unsigned long)sp, (unsigned long)bp);
	printf("***************************************************************\n");                                                                                       

	printf("Global init:\n");
	printf("Variable: 	%#lx\n", (unsigned long)&global_init_variable);
	printf("Pointer: 	%#lx\n", (unsigned long)&global_init_pointer);
	printf("Rrray: 		%#lx\n", (unsigned long) global_init_array);
	printf("Enum: 		%#lx\n", (unsigned long)&global_init_enum);
	printf("struct: 	%#lx\n", (unsigned long)&global_init_struct);
	printf("union: 		%#lx\n", (unsigned long)&global_init_union);

	printf("***************************************************************\n");                                                                                       
	printf("Global none init:\n");
	printf("Variable: 	%#lx\n", (unsigned long)&global_none_init_variable);
	printf("Pointer: 	%#lx\n", (unsigned long)&global_none_init_pointer);
	printf("Rrray: 		%#lx\n", (unsigned long) global_none_init_array);
	printf("Enum: 		%#lx\n", (unsigned long)&global_none_init_enum);
	printf("struct: 	%#lx\n", (unsigned long)&global_none_init_struct);
	printf("union: 		%#lx\n", (unsigned long)&global_none_init_union);
	printf("***************************************************************\n");                                                                                       

	printf("Local none init:\n");
	printf("Variable: 	%#lx\n", (unsigned long)&local_none_init_variable);
	printf("Pointer: 	%#lx\n", (unsigned long)&local_none_init_pointer);
	printf("Rrray: 		%#lx\n", (unsigned long)local_none_init_array);
	printf("Enum: 		%#lx\n", (unsigned long)&local_none_init_enum);
	printf("struct: 	%#lx\n", (unsigned long)&local_none_init_struct);
	printf("union: 		%#lx\n", (unsigned long)&local_none_init_union);
	printf("***************************************************************\n");                                                                                       

	printf("Local init:\n");
	printf("Variable: 	%#lx\n", (unsigned long)&local_init_variable);
	printf("Pointer: 	%#lx\n", (unsigned long)&local_init_pointer);
	printf("Rrray: 		%#lx\n", (unsigned long)local_init_array);
	printf("Enum: 		%#lx\n", (unsigned long)&local_init_enum);
	printf("struct: 	%#lx\n", (unsigned long)&local_init_struct);
	printf("union: 		%#lx\n", (unsigned long)&local_init_union);
	printf("***************************************************************\n");                                                                                       

	printf("Static Local none init:\n");
	printf("Variable: 	%#lx\n", (unsigned long)&static_local_none_init_variable);
	printf("Pointer: 	%#lx\n", (unsigned long)&static_local_none_init_pointer);
	printf("Rrray: 		%#lx\n", (unsigned long)static_local_none_init_array);
	printf("Enum: 		%#lx\n", (unsigned long)&static_local_none_init_enum);
	printf("struct: 	%#lx\n", (unsigned long)&static_local_none_init_struct);
	printf("union: 		%#lx\n", (unsigned long)&static_local_none_init_union);
	printf("***************************************************************\n");                                                                                       

	printf("Static Local init:\n");
	printf("Variable: 	%#lx\n", (unsigned long)&static_local_init_variable);
	printf("Pointer: 	%#lx\n", (unsigned long)&static_local_init_pointer);
	printf("Rrray: 		%#lx\n", (unsigned long)static_local_init_array);
	printf("Enum: 		%#lx\n", (unsigned long)&static_local_init_enum);
	printf("struct: 	%#lx\n", (unsigned long)&static_local_init_struct);
	printf("union: 		%#lx\n", (unsigned long)&static_local_init_union);
	printf("***************************************************************\n");                                                                                       

	return 0;
}
