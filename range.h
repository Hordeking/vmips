#ifndef __range_h__
#define __range_h__

/* Mapping types */
#define UNUSED 0    /* Not in use, maps no addrs. */
#define MALLOC 1    /* Malloc'd memory */
#define MMAP 2      /* Mmapped file */
#define DEVICE 3    /* See devicemap.h */
#define PROXY 4     /* This ProxyRange points to another Range object */
#define OTHER 5     /* Something else? */

class CPU;
class DeviceExc;

/* A doubly linked sorted list of memory mappings. */
class Range { 
protected:
	uint32 base;	/* beginning virt.machine addr represented by the Range */
	uint32 extent;	/* size of virt.machine address space */
	caddr_t address;  /* host machine's pointer to beginning of memory */
	Range *next;	/* next Range in list */
	Range *prev;	/* previous Range in list */
	int32 type;		/* See above for "Mapping types" */
	int perms;      /* MEM_READ, MEM_WRITE, etc. in accesstypes.h */

public:
	Range(uint32 b, uint32 e, caddr_t a, int32 t, int p, Range *n = NULL);
	virtual ~Range();
	virtual Range *insert(Range *newrange);
	virtual bool incorporates(uint32 addr);
	virtual char *descriptor_str(void);
	virtual void print(void);

	virtual uint32 getBase(void);
	virtual uint32 getExtent(void);
	virtual caddr_t getAddress(void);
	virtual int32 getType(void);
	virtual Range *getPrev(void);
	virtual void setPrev(Range *newPrev);
	virtual Range *getNext(void);
	virtual void setNext(Range *newNext);

	virtual int getPerms(void);
	virtual void setPerms(int newPerms);
	virtual bool canRead(uint32 offset);
	virtual bool canWrite(uint32 offset);

	virtual uint32 fetch_word(uint32 offset, int mode, DeviceExc *client);
	virtual uint16 fetch_halfword(uint32 offset, DeviceExc *client);
	virtual uint8 fetch_byte(uint32 offset, DeviceExc *client);
	virtual uint32 store_word(uint32 offset, uint32 data, DeviceExc *client);
	virtual uint16 store_halfword(uint32 offset, uint16 data,
		DeviceExc *client);
	virtual uint8 store_byte(uint32 offset, uint8 data, DeviceExc *client);
};

class ProxyRange: public Range
{
private:
	Range *realRange;
public:
	ProxyRange(Range *r, uint32 b, Range *n = NULL);

	virtual Range *getRealRange(void);

	virtual uint32 fetch_word(uint32 offset, int mode, DeviceExc *client);
	virtual uint16 fetch_halfword(uint32 offset, DeviceExc *client);
	virtual uint8 fetch_byte(uint32 offset, DeviceExc *client);
	virtual uint32 store_word(uint32 offset, uint32 data, DeviceExc *client);
	virtual uint16 store_halfword(uint32 offset, uint16 data,
		DeviceExc *client);
	virtual uint8 store_byte(uint32 offset, uint8 data, DeviceExc *client);

	virtual int getPerms(void);
	virtual void setPerms(int newPerms);
	virtual bool canRead(uint32 offset);
	virtual bool canWrite(uint32 offset);
};

#endif /* __range_h__ */
