#ifndef __debug_h__
#define __debug_h__

#include "sysinclude.h"
#include "deviceexc.h"

/* Instructions that gdb uses to set breakpoints. 
 * These are taken from gdb/mips-tdep.c, without modification.
 */
#define BIG_BREAKPOINT {0, 0x5, 0, 0xd}
#define LITTLE_BREAKPOINT {0xd, 0, 0x5, 0}

class CPU;
class Mapper;

class Debug : public DeviceExc {
private:
	bool debugger_shutdown;
	CPU *cpu;
	Mapper *mem;
	int signo;
	int listener;
	long threadno_step;
	long threadno_gen;
	uint32 rom_baseaddr;
	uint32 rom_nwords;
	uint8 *rom_bp_bitmap;

public:
	Debug();

	static void packet_push_word(char *packet, uint32 n);
	static void packet_push_byte(char *packet, uint8 n);
	static uint32 packet_pop_word(char **packet);
	static uint8 packet_pop_byte(char **packet);

	int setup(uint32 baseaddr, uint32 nwords);
	int serverloop(void);
	void attach(CPU *c, Mapper *m);
    void exception(uint16 excCode, int mode, int coprocno);

private:
	int setup_listener_socket(void);
	int set_nonblocking(int fd);
	void print_local_name(int s);
	int targetloop(void);
	char *rawpacket(char *str);
	char *error_packet(int error_code);
	char *signal_packet(int signal);
	char *target_kill(char *pkt);
	char *target_set_thread(char *pkt);
	char *target_read_registers(char *pkt);
	char *target_write_registers(char *pkt);
	char *target_read_memory(char *pkt);
	char *target_write_memory(char *pkt);
	uint8 single_step(void);
	char *target_continue(char *pkt);
	char *target_step(char *pkt);
	char *target_last_signal(char *pkt);
	char *target_unimplemented(char *pkt);
	int exccode_to_signal(int exccode);
	/* ROM breakpoint support */
	bool rom_breakpoint_exists(uint32 addr);
	void declare_rom_breakpoint(uint32 addr);
	void remove_rom_breakpoint(uint32 addr);
	bool address_in_rom(uint32 addr);
	void get_breakpoint_bitmap_entry(uint32 addr, uint8 *&entry, uint8 &bitno);
	bool is_breakpoint_insn(char *packetptr);
};

#endif /* __debug_h__ */
