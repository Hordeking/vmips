/* This code is only useful for compiling the boot monitor to
 * run in the host operating system. It should not be linked with
 * the ROM version of the boot monitor.
 */

#include "lib.h"

extern void *malloc (int size);
extern void entry (void);

int
main (int argc, char **argv)
{
  extern unsigned char *recv_buffer;

  recv_buffer = (unsigned char *) malloc (800000);
  if (!recv_buffer)
    {
      puts ("Can't allocate receive buffer");
      return 1;
    }

  set_echo (0);

  entry ();
  return 0;
}
