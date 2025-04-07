/*
 *  open_unlink.c
 *
 *  brif
 *  	open a file and no close, then delete 
 *  
 *  (C) 2025.04.02 <hkdywg@163.com>
 *
 *  This program is free software; you can redistribute it and/r modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 * */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

char write_buf[32*1024*1024] = {0x55, 0xaa};

int main(int argc, char *argv[])
{
    int fd = open("/test_dir/file", O_APPEND);
    write(fd, write_buf, 32*1024*1024);
    remove("/test_dir/file");
    while(1){
        sleep(1);
    }
	return 0;
}
