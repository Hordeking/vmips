#ifndef __serialhost_h__
#define __serialhost_h__

#include "sysinclude.h"

class SerialHost {
private:
	int fd;

public:
	void write_byte(char newData);
	int read_byte(size_t nbytes, uint8 *outbuf);
	bool dataAvailable(void);
	SerialHost(int f);
	SerialHost(FILE *fp);
};

#endif /* __serialhost_h__ */
