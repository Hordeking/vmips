#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "xmrcv.h"

#define MAXFILESIZE 800000

int fd;

int main(int argc, char **argv);

int receive_byte(unsigned char *ch) {
	return ((read(fd, ch, 1) < 1) ? -1 : 0);
}

int send_byte(unsigned char ch) {
	return ((write(fd, &ch, 1) < 1) ? -1 : 0);
}

int main(int argc, char **argv) {
	int size;
	unsigned char *buf = NULL;
	char *ttyfilename = argv[1];
	char *outfilename = argv[2];
	FILE *out = NULL;

	if (argc < 3) {
		printf("Must specify terminal name and output file name.\n");
		return 1;
	}

	/* Open the terminal */
	fd = open(ttyfilename, O_RDWR);
	if (fd < 0) {
		printf("Cannot open terminal %s - %s.\n", argv[1], strerror(errno));
		return 1;
	}

	/* Allocate receive buffer in RAM */
	buf = (unsigned char *) malloc(MAXFILESIZE);
	if (! buf) {
		printf("Cannot malloc space to store file.\n");
		return 1;
	}

	/* Receive the file. */
	printf("Ready to receive.\n");
	if (xmrcv(&size, buf) < 0) {
		printf("Receive failed.\n");
	} else {
		printf("Successfully received %d bytes.\n", size);
	}

	/* Write received data to file. */
	out = fopen(outfilename, "wb");
	if (! out) {
		printf("Cannot open output file for writing - %s.\n",strerror(errno));
		return 1;
	}
	if (fwrite(buf, sizeof(unsigned char), size, out) != size) {
		printf("Write error on output file - %s.\n",strerror(errno));
		return 1;
	}
	fclose(out);

	return 0;
}

