/* A sensible interface to MIPS cross compilation tools.
   Copyright 2001 Brian R. Gaeke.

This file is part of VMIPS.

VMIPS is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

VMIPS is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License along
with VMIPS; if not, write to the Free Software Foundation, Inc.,
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* For later: ways to get page size:
long sysconf(_SC_PAGE_SIZE);
or 
long sysconf(_SC_PAGESIZE);
or
int getpagesize(void);
*/

#include "sysinclude.h"

#define MAXARGV 500
#define TMPFNAMESIZE 26

int verbose = 0;
int dryrun = 0;

/* From Makefile */
const char pkgdatadir[] = PKGDATADIR;
char ccname[] = MIPSTOOLPREFIX "gcc";
char asname[] = MIPSTOOLPREFIX "as";
char ldname[] = MIPSTOOLPREFIX "ld";
char objcopyname[] = MIPSTOOLPREFIX "objcopy";
char objdumpname[] = MIPSTOOLPREFIX "objdump";
char endianflag[] = ENDIAN_FLAG;

/* ROMs are padded to this many bytes. I don't think this is strictly */
/* necessary and will probably go away eventually. */
long pagesz = 4096;

void usage(void)
{
	puts("usage:");
	puts(" vmipstool [ VT-FLAGS ] --compile [ FLAGS ] FILE.c -o FILE.o");
	puts(" vmipstool [ VT-FLAGS ] --preprocess [ FLAGS ] FILE");
	puts(" vmipstool [ VT-FLAGS ] --assemble [ FLAGS ] FILE.s -o FILE.o");
	puts
		(" vmipstool [ VT-FLAGS ] --link [ FLAGS ] FILE1.o ... FILEn.o -o PROG");
	puts(" vmipstool [ VT-FLAGS ] --make-rom PROG PROG.rom");
	puts(" vmipstool [ VT-FLAGS ] --disassemble-rom PROG.rom");
	puts(" vmipstool [ VT-FLAGS ] --disassemble PROG (or FILE.o)");
	puts("");
	puts("VT-FLAGS may include:");
	puts(" --help         Display this help message and exit.");
	puts(" --version      Display the version of vmipstool and exit.");
	puts(" --verbose      Echo commands as they are run.");
	puts
		(" --dry-run      Don't actually run anything; use with --verbose.");
	puts(" --ld-script=T  Use T as ld script (instead of default script);");
	puts("                use with --link.");
	puts("");
	puts("Report bugs to <vmips@dgate.org>.");
}

void maybe_echo(char **newargv)
{
	int i;
	if (verbose)
	{
		for (i = 0; newargv[i] != NULL; i++)
		{
			fprintf(stderr, "%s ", newargv[i]);
		}
		fprintf(stderr, "\n");
	}
}

int maybe_run(char **newargv)
{
	pid_t pid;
	int status, error = 0;

	if (!dryrun)
	{
		if ((pid = fork()) == 0)
			execv(newargv[0], newargv);
		else
		{
			waitpid(pid, &status, 0);
			if (WIFEXITED(status))
			{
				if (WEXITSTATUS(status) != 0)
				{
					error = 1;
				}
				else
				{
					error = 0;
				}
			}
			else
			{
				error = 1;
			}
		}
	}
	return error;
}

int echo_and_run_l(char *c, ...)
{
	va_list ap;
	char *newargv[MAXARGV];
	int newargc = 0;

	va_start(ap, c);
	newargv[newargc++] = c;
	while (c != NULL)
	{
		c = va_arg(ap, char *);
		newargv[newargc++] = c;
	}
	va_end(ap);

	maybe_echo(newargv);
	return maybe_run(newargv);
}

int echo_and_run_lv(char *c, ...)
{
	va_list ap;
	char *newargv[MAXARGV];
	char **originalargv;
	int newargc = 0;
	int i;

	va_start(ap, c);
	newargv[newargc++] = c;
	while (c != NULL)
	{
		c = va_arg(ap, char *);
		newargv[newargc++] = c;
	}
	newargc--;
	originalargv = va_arg(ap, char **);
	for (i = 0; originalargv[i] != NULL; i++)
	{
		newargv[newargc++] = originalargv[i];
	}
	newargv[newargc++] = NULL;
	va_end(ap);

	maybe_echo(newargv);
	return maybe_run(newargv);
}

int search_path(char *result, char *path, const char *target)
{
	FILE *f = NULL;
	char *dir = NULL;
	char trypath[PATH_MAX] = "";
	char *delims = ":";
	int found_it = 0;

	dir = strtok(path, delims);
	while (!found_it && (dir != NULL))
	{
		sprintf(trypath, "%s/%s", dir, target);
		if ((f = fopen(trypath, "r")) != NULL)
		{
			strcpy(result, trypath);
			fclose(f);
			found_it = 1;
		}
		else
		{
			dir = strtok(NULL, delims);
		}
	}
	return (found_it ? 0 : -1);
}

char ldscript_full_path[PATH_MAX];

int find_ldscript(void)
{
	/* Build the search path */
	char *ldscript_search_path;

	/* so that it can work without being installed */
	char rest_of_path[] =
		":.:..:./sample_code:../sample_code:../../sample_code";
	const char ldscript_name[] = "ld.script";

	ldscript_search_path =
		new char[strlen(rest_of_path) + strlen(pkgdatadir) + 2];
	strcpy(ldscript_search_path, pkgdatadir);
	strcat(ldscript_search_path, rest_of_path);

	/* Look for the ld.script */
	return search_path(ldscript_full_path, ldscript_search_path,
					   ldscript_name);
}

int copy_with_padded_blocks(char *in, char *out, long size)
{
	char *buff = new char[size];
	FILE *f = fopen(in, "rb");
	if (!f)
	{
		perror(in);
		return -1;
	}
	FILE *g = fopen(out, "wb");
	if (!g)
	{
		perror(out);
		return -1;
	}
	errno = 0;
	int readcount;
	while ((readcount = fread(buff, 1, size, f)) != 0)
	{
		if (readcount < size)
		{
			if (errno)
			{
				perror(in);
				fclose(f);
				fclose(g);
				return -1;
			}
			else
			{
				for (int i = readcount; i < size; i++)
				{
					buff[i] = '\0';
				}
			}
		}
		int writecount = fwrite(buff, 1, size, g);
		if (writecount < size)
		{
			perror(out);
			fclose(f);
			fclose(g);
			return -1;
		}
	}
	fclose(f);
	fclose(g);
	delete [] buff;
	return 0;
}

int swap(char *inputfname, char *outputfname)
{
	FILE *inputfp, *outputfp;
	char buff[4], buff2[4];
	int readcount, writecount;

	inputfp = fopen(inputfname, "rb");
	if (!inputfp)
	{
		perror(inputfname);
		return -1;
	}
	outputfp = fopen(outputfname, "wb");
	if (!outputfp)
	{
		perror(outputfname);
		return -1;
	}
	while ((readcount = fread(buff, 1, 4, inputfp)) > 0)
	{
		if (readcount < 4)
		{
			fprintf(stderr,
					"%s: warning: file does not end on a word boundary\n",
					inputfname);
			for (int i = readcount; i < 3; i++)
			{
				buff[i] = 0;
			}
		}
		buff2[0] = buff[3];
		buff2[1] = buff[2];
		buff2[2] = buff[1];
		buff2[3] = buff[0];
		if ((writecount = fwrite(buff2, 1, 4, outputfp)) != 4)
		{
			perror(outputfname);
			break;
		}
	}
	fclose(inputfp);
	fclose(outputfp);
	return 0;
}

int main(int argc, char **argv)
{
	int error = 0;
	int done = 0;
	char *option;
	char tmp[TMPFNAMESIZE];
	char *inputfile;
	char *outputfile;

	argv++, argc--;
	while (!done)
	{
		option = argv[0];
		argv++, argc--;

		if (option == NULL)
		{
			usage();
			error = 1;
			done = 1;
			break;
		}

		if (strcmp(option, "--version") == 0)
		{
			printf("vmipstool (VMIPS) %s\n", VERSION);
			done = 1;
		}
		else if (strcmp(option, "--help") == 0)
		{
			usage();
			done = 1;
		}
		else if (strcmp(option, "--verbose") == 0)
		{
			verbose = 1;
		}
		else if (strcmp(option, "--dry-run") == 0)
		{
			dryrun = 1;
		}
		else if (strncmp(option, "--ld-script=", 12) == 0)
		{
			FILE *f;

			strcpy(ldscript_full_path, &option[12]);
			if ((f = fopen(ldscript_full_path, "rb")) == NULL)
			{
				perror(ldscript_full_path);
				error = 1;
				done = 1;
			}
			else
			{
				fclose(f);
			}
		}
		else if (strcmp(option, "--compile") == 0)
		{
			error =
				echo_and_run_lv(ccname, endianflag, "-mno-abicalls", NULL,
								argv, NULL);
			done = 1;
		}
		else if (strcmp(option, "--assemble") == 0)
		{
			error = echo_and_run_lv(ccname, "-c", "-x", "assembler-with-cpp", endianflag, NULL, argv, NULL);
			done = 1;
		}
		else if (strcmp(option, "--preprocess") == 0)
		{
			error = echo_and_run_lv(ccname, "-E", NULL, argv, NULL);
			done = 1;
		}
		else if (strcmp(option, "--link") == 0)
		{
			if (find_ldscript() < 0)
			{
				fprintf(stderr, "vmipstool: can't find ld.script\n");
				error = 1;
			}
			else
			{
				error = echo_and_run_lv
					(ldname, endianflag, "-T", ldscript_full_path, NULL,
					 argv, NULL);
			}
			done = 1;
		}
		else if (strcmp(option, "--make-rom") == 0)
		{
			if (argc != 2)
			{
				fprintf(stderr,
						"vmipstool: not enough arguments to --make-rom mode\n");
				error = 1;
				usage();
			}
			else
			{
				sprintf(tmp, "/tmp/vmipstool-%d-1", (int) getpid());
				inputfile = argv[0];
				outputfile = argv[1];
				error =
					echo_and_run_l(objcopyname, "-O", "binary", inputfile,
								   tmp, NULL);
				if (error)
				{
					fprintf(stderr,
							"vmipstool: Error creating binary image of program.  Aborting.\n");
					done = 1;
					continue;
				}
				if (verbose)
				{
					printf
						("dd if=%s of=%s bs=%ld conv=sync > /dev/null 2>&1\n",
						 tmp, outputfile, pagesz);
				}
				if (copy_with_padded_blocks(tmp, outputfile, pagesz) < 0)
				{
					error = 1;
					fprintf(stderr,
							"vmipstool: Error copying program image to ROM file.  Aborting.\n");
				}
				else
				{
					if (verbose)
					{
						printf("rm %s\n", tmp);
					}
					unlink(tmp);
				}
			}
			done = 1;
		}
		else if (strcmp(option, "--disassemble-rom") == 0)
		{
			inputfile = argv[0];
			error = echo_and_run_l
				(objdumpname, "--disassemble-all", "--target=binary",
				 endianflag, "-m", "mips", inputfile, NULL);
			done = 1;
		}
		else if (strcmp(option, "--disassemble") == 0)
		{
			inputfile = argv[0];
			error = echo_and_run_l
				(objdumpname, "--disassemble", endianflag, inputfile, NULL);
			done = 1;
		}
		else if (strcmp(option, "--swap-words") == 0)
		{
			inputfile = argv[0];
			outputfile = argv[1];
			if (swap(inputfile, outputfile) < 0)
			{
				error = 1;
			}
			done = 1;
		}
		else
		{
			usage();
			error = 1;
			done = 1;
		}
	}
	return (error ? 1 : 0);
}
