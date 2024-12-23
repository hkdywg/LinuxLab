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

#define GUARD_PAGE 	(4096)

int main(int argc, char *argv[])
{
	extern char __executable_start[];
	extern char edata[];
	extern char etext[];
	extern char end[];
	unsigned long sp, bp;

	/* Bottom for stack */
	asm volatile ("mov %0, x29"
				  : "=r" (bp));

	/* Top for stack */
	asm volatile ("mov %0, sp"
				  : "=r" (sp));

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

	return 0;
}
