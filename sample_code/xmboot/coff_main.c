#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "coff.h"

int main(int argc, char **argv)
{
	unsigned char *buf = NULL;
	char *filename = NULL;
	FILE *fp = NULL;
	struct coff_info header;
	struct stat st;
	size_t len, rv;

	if (argc < 2) {
		fputs("usage: coff filename\n", stderr);
		return 1;
	}

	filename = argv[1];
	fp = fopen(filename, "rb");
	if (! fp) {
		fprintf(stderr, "opening %s: %s\n", filename, strerror(errno));
		return 1;
	}

	if (fstat(fileno(fp), &st) < 0) {
		fprintf(stderr, "stat %s: %s\n", filename, strerror(errno));
		fclose(fp);
		return 1;
	}
	len = (size_t) st.st_size;
	buf = (unsigned char *) malloc(len);
	if (! buf) {
		fprintf(stderr, "Can't malloc %u bytes\n", len);
		fclose(fp);
		return 1;
	}

	if ((rv = fread(buf, 1, len, fp)) != len) {
		fprintf(stderr, "reading %s: %u of %u bytes (%s)\n", 	
			filename, rv, len, strerror(errno));
		fclose(fp);
		return 1;
	}

	if (coff_analyze(buf, &header) < 0) {
		fputs("coff_analyze failed\n", stderr);
		fclose(fp);
		return 1;
	} else {
		print_coff_header(&header);
	}
	return 0;
}

