#include "debug.h"
#include "remotegdb.h"
#include "cpu.h"
#include "mapper.h"
#include "regnames.h"
#include "cpzeroreg.h"
#include "vmips.h"

extern vmips *machine;

Debug::Debug()
{
	/* Upon connecting to our socket, gdb will ask for the current
	 * signal; so we set the current signal to the breakpoint signal.
	 */
	signo = exccode_to_signal(Bp);
	debugger_shutdown = false;
	cpu = NULL;
	mem = NULL;
    listener = -1;
	threadno_step = -1;
	threadno_gen = -1;
	rom_baseaddr = 0;
	rom_nwords = 0;
	rom_bp_bitmap = NULL;
}

void
Debug::exception(uint16 excCode, int mode, int coprocno)
{
	/* not implemented yet */
	fprintf(stderr, "Debug::exception(%u, %d, %d)\n", excCode, mode,
		coprocno);
}

void
Debug::attach(CPU *c, Mapper *m)
{
	if (c) cpu = c;
	if (m) mem = m;
}

int
Debug::setup(uint32 baseaddr, uint32 nwords)
{
	/* Allocate space for one bit per ROM word in the ROM breakpoint
	 * bitmap (rom_bp_bitmap). Initially, all breakpoints are cleared.
	 */
	rom_bp_bitmap = new uint8[nwords / 8];
	for (uint32 i = 0; i < (nwords/8); i++) {
		rom_bp_bitmap[i] = 0;
	}
		
	rom_baseaddr = baseaddr;
	rom_nwords = nwords;

	/* Set up a TCP/IP socket to listen for a debugger connection. */
	listener = setup_listener_socket();
	if (listener >= 0) {
		/* Print out where we bound to. */
		print_local_name(listener);
		return 0;
	} else {
		return -1;
	}
}

/* True if a ROM breakpoint has been set for the instruction given in ADDR,
 * and false otherwise.
 */
bool
Debug::rom_breakpoint_exists(uint32 addr)
{
	uint8 *bitmap_entry;
	uint8 bitno;
	
	get_breakpoint_bitmap_entry(addr, bitmap_entry, bitno);
	if (! bitmap_entry) return false;
	return (*bitmap_entry & (1 << bitno)) != 0;
}

/* Set a ROM breakpoint for the instruction given in ADDR. */
void
Debug::declare_rom_breakpoint(uint32 addr)
{
	uint8 *bitmap_entry;
	uint8 bitno;
	
	get_breakpoint_bitmap_entry(addr, bitmap_entry, bitno);
	if (! bitmap_entry) return;
	*bitmap_entry |= (1 << bitno);
}

/* Unset a ROM breakpoint for the instruction given in ADDR. */
void
Debug::remove_rom_breakpoint(uint32 addr)
{
	uint8 *bitmap_entry;
	uint8 bitno;
	
	get_breakpoint_bitmap_entry(addr, bitmap_entry, bitno);
	if (! bitmap_entry) return;
	*bitmap_entry &= ~(1 << bitno);
}

/* True if ADDR is a virtual address within a known ROM block. This is pretty
 * lame right now; we should really ask the Mapper.
 */
bool
Debug::address_in_rom(uint32 addr)
{
	return !((addr < rom_baseaddr) || (addr > (rom_baseaddr + 4*rom_nwords)));
}

/* Given the address (ADDR) of an instruction, return the corresponding
 * entry in the ROM breakpoint bitmap (in ENTRY) and the corresponding
 * bit number in that entry (in BITNO).
 */
void
Debug::get_breakpoint_bitmap_entry(uint32 addr, uint8 *&entry, uint8 &bitno)
{
	uint32 wordno;

	if (! address_in_rom(addr)) {
		fprintf(stderr, "That doesn't look like a ROM address to me: %lx\n",
			addr);
		entry = NULL;
		return;
	}
	addr -= rom_baseaddr;
	wordno = addr >> 5;
	bitno = addr >> 2 & 0x07;
	entry = &rom_bp_bitmap[wordno];
}

/* Determine whether the packet pointer passed in points to a (serial-encoded)
 * GDB break instruction, and return true if this is the case.
 */
bool
Debug::is_breakpoint_insn(char *packetptr)
{
	int posn = 0;
#if defined(TARGET_BIG_ENDIAN)
	char break_insn[] = BIG_BREAKPOINT;
#elif defined(TARGET_LITTLE_ENDIAN)
	char break_insn[] = LITTLE_BREAKPOINT;
#endif
	int bytes = sizeof(break_insn);

	while (--bytes) {
		if (packet_pop_byte(&packetptr) != break_insn[posn++]) 
			return false;
	}
	return true;
}

void
Debug::packet_push_word(char *packet, uint32 n)
{
	char packetpiece[10];

#if defined(TARGET_LITTLE_ENDIAN)
	n = Mapper::swap_word(n);
#endif
	sprintf(packetpiece, "%08lx", n);
	strcat(packet, packetpiece);
} 

void
Debug::packet_push_byte(char *packet, uint8 n)
{
	char packetpiece[4];

	sprintf(packetpiece, "%02x", n);
	strcat(packet, packetpiece);
} 

uint32
Debug::packet_pop_word(char **packet)
{
	char valstr[10];
	char *q, *p;
	int i;
	uint32 val;

	p = *packet;
	q = valstr;
	for (i = 0; i < 8; i++) {
		*q++ = *p++;
	}
	*q++ = '\0';
	val = strtoul(valstr, NULL, 16);
#if defined(TARGET_LITTLE_ENDIAN)
	val = Mapper::swap_word(val);
#endif
    *packet = p;
	return val;
}

uint8
Debug::packet_pop_byte(char **packet)
{
	char valstr[4];
	char *q, *p;
	int i;
	uint8 val;

	p = *packet;
	q = valstr;
	for (i = 0; i < 2; i++) {
		*q++ = *p++;
	}
	*q++ = '\0';
	val = (uint8) strtoul(valstr, NULL, 16);
    *packet = p;
	return val;
}

int
Debug::setup_listener_socket(void)
{
	int sock;
	struct sockaddr_in addr;
	int value;

	sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		perror("socket");
		return -1;
	}
	addr.sin_family = AF_INET;
	addr.sin_port = 0;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	value = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &value,
    	sizeof(value)) < 0) {
		perror("setsockopt SO_REUSEADDR");
		return -1;
	}
	if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		perror("bind");
		return -1;
	}
	if (listen(sock, 1) < 0) {
		perror("listen");
		return -1;
	}
	return sock;
}

int
Debug::set_nonblocking(int fd)
{
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
		perror("fcntl: set O_NONBLOCK");
		return -1;
	}
	return 0;
}

void
Debug::print_local_name(int s)
{
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);

    if (getsockname(s, (struct sockaddr *) &addr, &addrlen) < 0) {
		perror("getsockname");
		return;
	}
	fprintf(stderr,
		"Use this command to attach debugger: target remote %s:%u\n",
		inet_ntoa(addr.sin_addr),
		ntohs(addr.sin_port));
}

int
Debug::serverloop(void)
{
	int clientno = 0;
	struct sockaddr_in clientaddr;
	int clientsock;
	socklen_t clientaddrlen = sizeof(clientaddr);
	extern int remote_desc;

#ifdef BROKEN
	/* FIXME: What to do here?? */
	fprintf(stderr, "Setting signal handler.\n");
	signal(SIGTERM, sig_handler);
	signal(SIGINT, sig_handler);
	signal(SIGHUP, sig_handler);
#endif

	while (! debugger_shutdown) {
		/* Block until a connection is received */
		fprintf(stderr, "Waiting for connection from debugger.\n");
		clientsock = accept(listener, (struct sockaddr *) &clientaddr,
			&clientaddrlen);

		/* Set the client socket nonblocking */
		set_nonblocking(clientsock);

		remote_desc = clientsock;

		targetloop();
		close(clientsock);
		clientno++;
	}
	close(listener);
	return 0;
}

int
Debug::targetloop(void)
{
	int packetno = 0;
	char buf[728];
	char *result = NULL;

	while (! debugger_shutdown) {
		printf("Waiting for packet %d\n", packetno);
		getpkt(buf, 1);
		switch(buf[0]) {
			case 'g': result = target_read_registers(buf); break;
			case 'G': result = target_write_registers(buf); break;
			case 'm': result = target_read_memory(buf); break;
			case 'M': result = target_write_memory(buf); break;
			case 'c': result = target_continue(buf); break;
			case 's': result = target_step(buf); break;
			case 'k': result = target_kill(buf); break;
			case 'H': result = target_set_thread(buf); break;
			case '?': result = target_last_signal(buf); break;
			default:  result = target_unimplemented(buf); break;
		}
		packetno++;
		putpkt(result);
		free(result);
	}
	return 0;
}

char *
Debug::target_kill(char *pkt)
{
	fprintf(stderr, "STUB: Kill\n");

	debugger_shutdown = true;
	return rawpacket("OK");
}

char *
Debug::target_set_thread(char *pkt)
{
	int thread_for_step = (pkt[1] == 'c');
	long threadno;

	threadno = strtol(&pkt[2], NULL, 0);
	fprintf(stderr, "STUB: Set thread for %s to %ld\n",
		(thread_for_step ? "step/continue" : "general ops"), threadno);
	if (thread_for_step) {
		threadno_step = threadno;
	} else {
		threadno_gen = threadno;
	}
	return rawpacket("OK");
}

char *
Debug::target_read_registers(char *pkt)
{
	fprintf(stderr, "STUB: Read registers\n");

	return cpu->debug_registers_to_packet();
}

char *
Debug::rawpacket(char *str)
{
	char *packet;

	packet = (char *) malloc((1 + strlen(str)) * sizeof(char));
	strcpy(packet, str);
	return packet;
}

char * 
Debug::target_write_registers(char *pkt)
{
	fprintf(stderr, "STUB: Write registers [%s]\n", &pkt[1]);
	
	cpu->debug_packet_to_registers(&pkt[1]);

	return rawpacket("OK");
}

char *
Debug::error_packet(int error_code)
{
	char str[10];
	sprintf(str, "E%02x", error_code);
	return rawpacket(str);
}

char *
Debug::signal_packet(int signal)
{
	char str[10];
	sprintf(str, "S%02x", signal);
	return rawpacket(str);
}

char * 
Debug::target_read_memory(char *pkt)
{
	char *addrstr = &pkt[1];
	char *lenstr = strchr(pkt, ',');
	uint32 addr, len;
	char *packet;

	if (! lenstr) {
		fprintf(stderr, "STUB: Malformed read memory request (no ,) [%s]\n",
			&pkt[1]);
		return error_packet(1);
	}
	lenstr[0] = 0;
	lenstr++;
	fprintf(stderr, "STUB: read memory addr=%s len=%s\n",addrstr,lenstr);

	addr = strtoul(addrstr, NULL, 16);
	len = strtoul(lenstr, NULL, 16);
	
	packet = (char *) malloc((2 * len + 1) * sizeof(char));
	packet[0] = '\0';
	
	cpu->debug_fetch_region(addr, len, packet, this);

	return packet;
}

char * 
Debug::target_write_memory(char *pkt)
{
	char *addrstr = &pkt[1];
	char *lenstr = strchr(pkt, ',');
	char *datastr = strchr(pkt, ':');
	uint32 addr, len;
	if (! lenstr) {
		fprintf(stderr, "STUB: Malformed write memory request (no ,) [%s]\n",
			&pkt[1]);
		return error_packet(1);
	}
	lenstr[0] = 0;
	lenstr++;
	if (! datastr) {
		fprintf(stderr, "STUB: Malformed write memory request (no :) [%s]\n",
			&pkt[1]);
		return error_packet(1);
	}
	datastr[0] = 0;
	datastr++;
	fprintf(stderr, "STUB: write memory addr=%s len=%s data=%s\n",
		addrstr, lenstr, datastr);

	addr = strtoul(addrstr, NULL, 16);
	len = strtoul(lenstr, NULL, 16);

	if ((len == 4) && address_in_rom(addr) && is_breakpoint_insn(datastr)) {
		fprintf(stderr, "ROM breakpoint SET at %lx\n", addr);
		declare_rom_breakpoint(addr);
	} else if ((len == 4) && address_in_rom(addr) &&
	           rom_breakpoint_exists(addr) && !is_breakpoint_insn(datastr)) {
		fprintf(stderr, "ROM breakpoint CLEARED at %lx\n", addr);
		remove_rom_breakpoint(addr);
	} else {
		cpu->debug_store_region(addr, len, datastr, this);
	}

	return rawpacket("OK");
}

uint8
Debug::single_step(void)
{
	if (rom_breakpoint_exists(cpu->debug_get_pc())) {
		return Bp; /* Simulate hitting the breakpoint. */
	}
	machine->periodic();
	return cpu->pending_exception();
}

char * 
Debug::target_continue(char *pkt)
{
	char *addrstr = &pkt[1];
	uint32 addr;
	int exccode;

	if (! addrstr[0]) {
		fprintf(stderr, "STUB: continue from last addr\n");
	} else {
		fprintf(stderr, "STUB: continue from addr=%s\n", addrstr);
		addr = strtoul(addrstr, NULL, 16);
		cpu->debug_set_pc(addr);
	}
	do { /* nothing */; } while ((exccode = single_step()) == 0);
	signo = exccode_to_signal(exccode);
	return signal_packet(signo);
}

char * 
Debug::target_step(char *pkt)
{
	char *addrstr = &pkt[1];
	uint32 addr;
	int exccode;

	if (! addrstr[0]) {
		fprintf(stderr, "STUB: step from last addr\n");
	} else {
		fprintf(stderr, "STUB: step from addr=%s\n", addrstr);
		addr = strtoul(addrstr, NULL, 16);
		cpu->debug_set_pc(addr);
	}
	if ((exccode = single_step()) != 0) {
		signo = exccode_to_signal(exccode);
	}
	return signal_packet(signo);
}

char * 
Debug::target_last_signal(char *pkt)
{
	return signal_packet(signo);
}

char * 
Debug::target_unimplemented(char *pkt)
{
	fprintf(stderr, "STUB: unimplemented request [%s]\n", pkt);
	return rawpacket("");
}

/* Translates between MIPS exception codes and Unix signals (which GDB
 * likes better.)
 */
int
Debug::exccode_to_signal(int exccode)
{
	const int signos[] = { 2,  /* Int -> SIGINT */
	 11, 11, 11, 11, 11,       /* Mod, TLBL, TLBS, AdEL, AdES -> SIGSEGV */
	 7, 7,                     /* IBE, DBE -> SIGBUS */
	 5, 5,                     /* Sys, Bp -> SIGTRAP */
	 4,                        /* RI -> SIGILL */
	 8, 8 };                   /* CpU, OV -> SIGFPE */
	const int defaultsig = 1;  /* others -> SIGHUP */
  
	if (exccode < Int || exccode > Ov) {
		return defaultsig; /* SIGHUP */
	} else {
		return signos[exccode];
	}
}
