#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	char *buffer;
	while(1) {
		buffer = malloc(267);		
		free(buffer);

		buffer = malloc(5000);		
		free(buffer);

		buffer = malloc(5 * 1024 * 1024);		
		free(buffer);
	}

	return 0;
}
