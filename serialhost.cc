#include "serialhost.h"
#include <unistd.h>
#include <stdio.h>

void SerialHost::write_byte(char newData)
{
	write(fd, &newData, sizeof(char));
}

int SerialHost::read_byte(size_t nbytes, uint8 *outbuf)
{
    if (read(fd, outbuf, nbytes * sizeof(uint8)) != (ssize_t)nbytes) {
		return -1;
	}
	return 0;
}

bool SerialHost::dataAvailable(void)
{
	struct timeval zero = {0, 0};
	fd_set to_read;
	FD_ZERO(&to_read);
	FD_SET(fd, &to_read);
	return (select(fd+1, &to_read, NULL, NULL, &zero) == 1);
}

SerialHost::SerialHost(int f)
{
	fd = f;
}

SerialHost::SerialHost(FILE *fp)
{
	fd = fileno(fp);
}

