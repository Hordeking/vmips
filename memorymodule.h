#ifndef __memorymodule_h__
#define __memorymodule_h__

class MemoryModule {
public:
    caddr_t addr;
    uint32 len;
    MemoryModule(size_t size);
    ~MemoryModule();
	void print(void);
};

#endif /* __memorymodule_h__ */
