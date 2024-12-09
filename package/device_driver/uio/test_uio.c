#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>

#define UIO_DEV_NAME "/dev/uio0"
#define UIO_ADDR 	"/sys/class/uio/uio0/maps/map0/addr"
#define UIO_SIZE 	"/sys/class/uio/uio0/maps/map0/size"

static char uio_addr_buf[32];
static char uio_size_buf[32];


int main(int argc, char *argv[])
{
	int fd_uio, fd_addr, fd_size;
	int irq_en, uio_size;
	void *uio_addr, *access_addr;

	/* uio interrupt control */
	fd_uio = open(UIO_DEV_NAME, O_RDWR);
	irq_en = atoi(argv[1]);
	if(irq_en == 0 || irq_en == 1)
		write(fd_uio, &irq_en, sizeof(int));

	/**/
	fd_addr = open(UIO_ADDR, O_RDONLY);
	fd_size = open(UIO_SIZE, O_RDONLY);

	read(fd_addr, uio_addr_buf, sizeof(uio_addr_buf));
	read(fd_size, uio_size_buf, sizeof(uio_size_buf));

	uio_addr = (void *)strtoul(uio_addr_buf, NULL, 0);
	uio_size = (int)strtol(uio_size_buf, NULL, 0);

	access_addr = mmap(NULL, uio_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_uio, 0);
	printf("device address %p (length: %d) virtual adress: %p\n", uio_addr, uio_size, access_addr);

	munmap(access_addr, uio_size);

	close(fd_size);
	close(fd_addr);
	close(fd_uio);

	return 0;
}
