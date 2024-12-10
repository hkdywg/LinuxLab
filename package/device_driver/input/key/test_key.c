#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <string.h>
#include <linux/input.h>

/* input sysfs direct */
#define INPUT_SYS_PATH "/sys/devices/platform/input_key/input"
/* input direct pn /dev/input */
static char input_dir[128] = "/dev/input";

int main(int argc, char *argv[])
{
	struct input_event ev;
	struct dirent *ptr;
	char events_dir[128];
	int find = 0;
	DIR *dir;
	int fd, rvl;

	if((dir = opendir(INPUT_SYS_PATH)) == NULL) {
		printf("Open %s dirent err\n", INPUT_SYS_PATH);
		return -1;
	}

	while((ptr = readdir(dir)) != NULL) {
		if(ptr->d_type == DT_DIR) {
			if(!strncmp(ptr->d_name, "input", strlen("input"))) {
				strcpy(events_dir, INPUT_SYS_PATH);
				strcat(events_dir, "/");
				strcat(events_dir, ptr->d_name);
				find = 1;
				break;
			}
		}
	}

	close(dir);
	if(!find) {
		printf("Error: can't find %s/input\n", INPUT_SYS_PATH);
		return -1;
	}
	find = 0;

	if((dir = opendir(events_dir)) == NULL) {
		printf("Open %s dirent err\n", events_dir);
		return -1;
	}

	while((ptr = readdir(dir)) != NULL) {
		if(ptr->d_type == DT_DIR) {
			if(!strncmp(ptr->d_name, "event", strlen("event"))) {
				strcpy(events_dir, ptr->d_name);
				printf("event_dir = %s, ptr->d_name = %s", events_dir, ptr->d_name);
				find = 1;
				break;
			}
		}
	}

	close(dir);
	if(!find) {
		printf("Error: can't find events dirent\n");
		return -1;
	}

	strcat(input_dir, "/");
	strcat(input_dir, events_dir);

	fd = open(input_dir, O_RDWR);
	if(fd < 0) {
		printf("unable to open %s\n", input_dir);
		return -1;
	}

	while(1) {
		memset(&ev, 0, sizeof(struct input_event));
		rvl = read(fd, &ev, sizeof(struct input_event));
		if(rvl != sizeof(struct input_event)) {
			printf("can't read event frame\n");
			close(fd);
			return rvl;
		}
		printf("------------------\n");
		printf("type:		%u\n", ev.type);
		printf("code:		%u\n", ev.code);
		printf("value:		%u\n", ev.value);

	}

	close(fd);
	return 0;
}
