#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

int main(int argc, char **argv)
{
	int fd;
	if (argc < 2) {
		printf("Must specify terminal name and file name to send.\n");
		return 1;
	}
	fd = open(argv[1], O_RDWR);
	if (fd<0) {perror(argv[1]); return 1;}
	close(0);
	close(1);
	if (dup2(fd, 0)<0) {perror("dup2 0"); return 1;}
	if (dup2(fd, 1)<0) {perror("dup2 1"); return 1;}
	execl("/usr/bin/sx", "sx", argv[2], NULL);
}
