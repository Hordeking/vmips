/* This is a simplistic test suite for the little C library.
 * 
 * It is a good idea to put some simplistic tests here for any function
 * you add to the little C library.
 *
 * $Id: libtest.c,v 1.5 2000/11/11 10:50:06 brg Exp $
 */

#ifdef USE_STANDARD_LIBRARY
# include <stdio.h>
# define puts_nonl(X) fputs(X, stdout)
#else /* USE_STANDARD_LIBRARY */
# include "lib.h"
#endif /* USE_STANDARD_LIBRARY */

int
main (int argc, char **argv)
{
  int fail, i;
  char buf[200];

  /* test puts */
  puts ("ok puts");

  /* test printf */
  printf("o%c pr%cntf", 'k', 'i');
  printf("\n");

  /* test puts_nonl */
  puts_nonl ("ok puts_nonl\n");

  /* test putchar */
  putchar ('o');
  putchar ('k');
  putchar (' ');
  putchar ('p');
  putchar ('u');
  putchar ('t');
  putchar ('c');
  putchar ('h');
  putchar ('a');
  putchar ('r');
  putchar ('\n');

  /* test getchar */
  fail = 0;
#ifndef USE_STANDARD_LIBRARY
  set_echo (0);
#endif /* USE_STANDARD_LIBRARY */
  if (getchar () != 'x')
    {
      puts ("[lost x]");
      fail++;
    }
  if (getchar () != '\n')
    {
      puts ("[lost newline]");
      fail++;
    }
  if (fail)
    {
      puts ("fail getchar");
    }
  else
    {
      puts ("ok getchar");
    }

  /* test strlen */
  fail = 0;
  if (strlen ("") != 0)
    {
      puts ("[empty]");
      fail++;
    }
  if (strlen ("abcdef") != 6)
    {
      puts ("[non-empty]");
      fail++;
    }
  if (fail)
    {
      puts ("fail strlen");
    }
  else
    {
      puts ("ok strlen");
    }

  /* test strcmp */
  fail = 0;
  if (strcmp ("less", "lesser") >= 0)
    {
      puts ("[len=4 vs len=6]");
      fail++;
    }
  if (strcmp ("lesseq", "lesser") >= 0)
    {
      puts ("[q vs r]");
      fail++;
    }
  if (strcmp ("lesser", "lesser") != 0)
    {
      puts ("[r vs r]");
      fail++;
    }
  if (strcmp ("lesser", "lesseq") <= 0)
    {
      puts ("[r vs q]");
      fail++;
    }
  if (strcmp ("lesser", "less") <= 0)
    {
      puts ("[len=6 vs len=4]");
      fail++;
    }
  if (fail)
    {
      puts ("fail strcmp");
    }
  else
    {
      puts ("ok strcmp");
    }

  /* test gets */
  fail = 0;
  gets (buf);
  if (strcmp (buf, "This is a test of the emergency broadcasting system.") !=
      0)
    {
      puts ("[comparison fails]");
      fail++;
    }
  if (fail)
    {
      puts ("fail gets");
    }
  else
    {
      puts ("ok gets");
    }

  /* test isspace */
  fail = 0;
  if (!isspace (' '))
    {
      puts ("[whitespace]");
      fail++;
    }
  if (isspace ('a'))
    {
      puts ("[non-whitespace]");
      fail++;
    }
  if (fail)
    {
      puts ("fail isspace");
    }
  else
    {
      puts ("ok isspace");
    }

  /* test isdigit */
  fail = 0;
  if (!isdigit ('0'))
    {
      puts ("[0]");
      fail++;
    }
  if (!isdigit ('1'))
    {
      puts ("[1]");
      fail++;
    }
  if (!isdigit ('2'))
    {
      puts ("[2]");
      fail++;
    }
  if (!isdigit ('3'))
    {
      puts ("[3]");
      fail++;
    }
  if (!isdigit ('4'))
    {
      puts ("[4]");
      fail++;
    }
  if (!isdigit ('5'))
    {
      puts ("[5]");
      fail++;
    }
  if (!isdigit ('6'))
    {
      puts ("[6]");
      fail++;
    }
  if (!isdigit ('7'))
    {
      puts ("[7]");
      fail++;
    }
  if (!isdigit ('8'))
    {
      puts ("[8]");
      fail++;
    }
  if (!isdigit ('9'))
    {
      puts ("[9]");
      fail++;
    }
  if (isdigit (' '))
    {
      puts ("[space]");
      fail++;
    }
  if (isdigit ('a'))
    {
      puts ("[alpha]");
      fail++;
    }
  if (fail)
    {
      puts ("fail isdigit");
    }
  else
    {
      puts ("ok isdigit");
    }

  /* test strtol */
  fail = 0;
  if (strtol ("0x1234", NULL, 0) != 0x1234)
    {
      puts ("[0x1234 base=0]");
      fail++;
    }
  if (strtol ("1234", NULL, 16) != 0x1234)
    {
      puts ("[1234 base=16]");
      fail++;
    }
  if (strtol ("01234", NULL, 0) != 01234)
    {
      puts ("[01234 base=0]");
      fail++;
    }
  if (strtol ("1234", NULL, 8) != 01234)
    {
      puts ("[1234 base=8]");
      fail++;
    }
  if (strtol ("1234", NULL, 0) != 1234)
    {
      puts ("[1234 base=0]");
      fail++;
    }
  if (strtol ("1234", NULL, 10) != 1234)
    {
      puts ("[1234 base=10]");
      fail++;
    }
  if (strtol ("-1234", NULL, 0) != -1234)
    {
      puts ("[-1234 base=0]");
      fail++;
    }
  if (strtol ("-1234", NULL, 10) != -1234)
    {
      puts ("[-1234 base=10]");
      fail++;
    }
  if (fail)
    {
      puts ("fail strtol");
    }
  else
    {
      puts ("ok strtol");
    }

  /* test strcpy */
  fail = 0;
  strcpy (buf, "ab");
  if (!((buf[0] == 'a') && (buf[1] == 'b') && (buf[2] == '\0')))
    fail++;
  strcpy (&buf[3], buf);
  if (!((buf[0] == 'a') && (buf[1] == 'b') && (buf[2] == '\0') &&
	(buf[3] == 'a') && (buf[4] == 'b') && (buf[5] == '\0')))
    fail++;
  if (fail)
    {
      puts ("fail strcpy");
    }
  else
    {
      puts ("ok strcpy");
    }

  /* test memmove */
  fail = 0;
  strcpy (buf, "xxxabcyyy");

  memmove (&buf[5], &buf[3], 3);
  if (!((buf[5] == 'a') && (buf[6] == 'b') && (buf[7] == 'c')))
    fail++;

  memmove (&buf[4], &buf[5], 3);
  if (!((buf[4] == 'a') && (buf[5] == 'b') && (buf[6] == 'c')))
    fail++;

  if (fail)
    {
      puts ("fail memmove");
    }
  else
    {
      puts ("ok memmove");
    }

  return 0;
}
