/*
 *  test_cma_base.c
 *
 *  brif
 *  	Contiguous Memory Allocate Application
 *  
 *  (C) 2024.12.12 <hkdywg@163.com>
 *
 *  This program is free software; you can redistribute it and/r modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 * */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#define CMA_MEM_ALLOCATE 	_IOW('m', 1, unsigned int)
#define CMA_MEM_RELEASE 	_IOW('m', 2, unsigned int)

#define CMA_DEVICE 			"/dev/cma_demo"

struct cma_demo_region
{
	unsigned long virt;
	unsigned long phys;
	unsigned long offset;
	unsigned long length;
};

int main(int argc, char *argv[])
{
	struct cma_demo_region region;
	void *base;
	int fd;

	if((fd = open(CMA_DEVICE, O_RDWR, 0)) < 0) {
		printf("Can't open %s\n", CMA_DEVICE);
		return -1;
	}

	memset(&region, 0, sizeof(struct cma_demo_region));
	region.length = 1 << 20;
	region.phys = 0;	/* auto phys address */

	if(ioctl(fd, CMA_MEM_ALLOCATE, &region) < 0) {
		printf("CMA_MEM_ALLOCATE: ioctl failed\n");
		return -1;
	}

	base = mmap(NULL, region.length, PROT_READ | PROT_WRITE, 
				MAP_SHARED, fd, region.phys);

	if(base == MAP_FAILED) {
		printf("CMA_MAP: mmap failed\n");
		close(fd);
		return -1;
	}

	strcpy(base, "test case of cma");
	printf("cma region: %s\n", (char *)base);
	printf("phys: %#lx - %#lx\n", region.phys, region.phys + region.length);
	printf("virt: %#lx - %#lx\n", (unsigned long)base, (unsigned long)base + region.length);

	munmap(base, region.length);

	if(ioctl(fd, CMA_MEM_RELEASE, &region) < 0) {
		printf("CMA_MEM_RELEASE: ioctl failed\n");
		close(fd);
		return -1;
	}

	if(ioctl(fd, CMA_MEM_RELEASE, &region) < 0) {
		printf("CMA_MEM_RELEASE: ioctl failed\n");
		close(fd);
		return -1;
	}

	close(fd);

	return 0;
}
