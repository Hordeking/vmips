/* Register offsets for: a SPIM-compatible console device. */

#ifndef __spimconsreg_h__
#define __spimconsreg_h__

/* register offsets */
#define KEYBOARD_1_CONTROL   0x00
#define KEYBOARD_1_DATA      0x04
#define DISPLAY_1_CONTROL    0x08
#define DISPLAY_1_DATA       0x0C
#define KEYBOARD_2_CONTROL   0x10
#define KEYBOARD_2_DATA      0x14
#define DISPLAY_2_CONTROL    0x18
#define DISPLAY_2_DATA       0x1C
#define CLOCK_CONTROL        0x20

/* bits in control regs */
#define CTL_IE				0x00000002
#define CTL_RDY				0x00000001

#endif /* __spimconsreg_h__ */
