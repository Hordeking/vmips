#include "sysinclude.h"
#include "vmips.h"

extern "C" { 

#include "bfd.h"
#include "dis-asm.h"

}

static struct disassemble_info disasm_info;

void
setup_disassembler(FILE *stream)
{
	/* Set up for libopcodes. */
	INIT_DISASSEMBLE_INFO(disasm_info, stream, fprintf);
	disasm_info.buffer_length = 4;
}

void
call_disassembler(uint32 pc, uint32 instr)
{
	/* Point libopcodes at the instruction... */
	disasm_info.buffer_vma = pc;
	disasm_info.buffer = (bfd_byte *) &instr;

	/* Disassemble the instruction, which is in *host* byte order. */
#if defined(WORDS_BIGENDIAN)
	print_insn_big_mips(pc, &disasm_info);
#else
	print_insn_little_mips(pc, &disasm_info);
#endif
	putc('\n', (FILE *)disasm_info.stream);   /* End the line. */
}
