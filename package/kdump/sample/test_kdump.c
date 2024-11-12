#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>


#define DEMO_DEV_NAME "/dev/my_demo_drv"
#define MYDEV_CMD_GET_BUFSIZE 1


int main(int argc, char *argv[])
{
	int fd;
	size_t len;
	int *mmap_buffer;

	fd = open(DEMO_DEV_NAME, O_RDWR);

	ioctl(fd, MYDEV_CMD_GET_BUFSIZE, &len);

	mmap_buffer = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	munmap(mmap_buffer, len);
	close(fd);

	return 0;
}
