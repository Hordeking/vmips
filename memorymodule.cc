#include "sysinclude.h"
#include "memorymodule.h"

MemoryModule::MemoryModule(size_t size)
{
	len = size;
	addr = (caddr_t) calloc(len/4, 4);
	if (!addr) {
		cerr << "Abort: out of memory allocating " << len/4 << " bytes" << endl;
		abort();
	}
}

void
MemoryModule::print(void)
{
	printf("([%lx] %lx)",(unsigned long) addr,len);
}

MemoryModule::~MemoryModule()
{
	free(addr);
}

