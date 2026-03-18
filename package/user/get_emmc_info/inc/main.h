#ifndef MAIN_H_
#define MAIN_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include <signal.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>
#include <assert.h>
#include <linux/fs.h> 
#include <stdbool.h>

#define Version "V1.0.0"

#ifndef Debug_pr

#define Debug_pr printf

#endif

enum eMMC_type
{
    type_0 =0, 
    type_bw, 
    type_jbl, 
    type_ky, 
    type_Max
};

#endif /*MAIN_H_*/

