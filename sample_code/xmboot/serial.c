#include "spimconsreg.h"

#define IOBASE 0xa2000000
#define IS_READY(ctrl) (((*(ctrl)) & CTL_RDY) != 0)

volatile long *display_data_reg = (long *)(IOBASE+DISPLAY_1_DATA);
volatile long *display_control_reg = (long *)(IOBASE+DISPLAY_1_CONTROL);
volatile long *keyboard_data_reg = (long *)(IOBASE+KEYBOARD_1_DATA);
volatile long *keyboard_control_reg = (long *)(IOBASE+KEYBOARD_1_CONTROL);

static unsigned char
internal_read_byte (void)
{
  do
    {
      0;
    }
  while (!IS_READY (keyboard_control_reg));
  return (char) *keyboard_data_reg;
}

static void
internal_write_byte (char out)
{
  do
    {
      0;
    }
  while (!IS_READY (display_control_reg));
  *display_data_reg = (long) out;
}

int
read (int fd, unsigned char *buf, int count)
{
  int i;
  char *b = buf;

  if (fd != 0)
    return 0;
  for (i = 0; i < count; i++)
    {
      *b++ = internal_read_byte ();
    }
  return count;
}

int
write (int fd, unsigned char *buf, int count)
{
  int i;
  char *b = buf;

  if (fd != 1)
    return 0;
  for (i = 0; i < count; i++)
    {
      internal_write_byte (*b++);
    }
  return count;
}
