/* Command-line and preferences-file options processing.
   Copyright 2001, 2003 Brian R. Gaeke.

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

#include "options.h"
#include "optiontbl.h"
#include "error.h"

Options::Options(int argc, char **argv)
{
	int x;

	for (x = 0; x < TABLESIZE; x++) {
		table[x].name = NULL;
		table[x].type = 0;
		table[x].value.num = 0;
	}
	process_options(argc, argv);
}

int
Options::tilde_expand(char *filename)
{
	struct passwd *pw;
	char buf[PATH_MAX], *trailer = NULL;

	if (filename[0] != '~') {
		return 0;
	}
	filename[0] = '\0';
	trailer = strchr(&filename[1],'/');
	if (trailer) {
		*trailer++ = '\0';
	}
	/* &filename[1] now consists of mumble\0 where mumble = zero or more chars
	 * representing a username
	 */
	if (strlen(&filename[1]) == 0) {
		pw = getpwuid(getuid());
	} else {
		pw = getpwnam(&filename[1]);
	}
	if (!pw) {
		return -1;
	}
	if (trailer) {
		sprintf(buf,"%s/%s",pw->pw_dir,trailer);
	} else {
		strcpy(buf,pw->pw_dir);
	}
	strcpy(filename, buf);
	return 0;
}

void
Options::process_defaults(void)
{
	char **opt;

	for (opt = defaults_table; *opt; opt++) {
		process_one_option(*opt);
	}
	/* ttydev gets a special default, if the OS understands ttyname(0). */
	char *ttydev_default = ttyname(0);
	if (ttydev_default != NULL) {
		set_str_option("ttydev", ttydev_default);
	}
#ifdef OPTIONS_DEBUG
	for (Option *o = nametable; o->type; o++) {
		if (option(o->name) == NULL) {
			fprintf(stderr, "Bug: Option `%s' has no compiled-in default.\n",
				o->name);
		}
	}
#endif /* OPTIONS_DEBUG */
}

void
Options::set_str_option(char *key, char *value)
{
	Option *o = optstruct(key, true);

#ifdef OPTIONS_DEBUG
	if (options_debug)
		fprintf(stderr, "STR option <%s,%s>\n",key,value);
#endif
	if (find_option_type(key) != STR) {
		error("unknown string variable %s has string value", key);
		return;
	}
	if (!o) {
		fprintf(stderr,"Bug: Option table filled up trying to set <%s,%s>!\n",
			key,value);
		return;
	}
	o->name = strdup(key);
	o->type = STR;
	o->value.str = strdup(value);
}

void
Options::set_num_option(char *key, uint32 value)
{
	Option *o = optstruct(key, true);

#ifdef OPTIONS_DEBUG
	if (options_debug)
		fprintf(stderr, "NUM option <%s,%u[0x%x]>\n",key,value,value);
#endif
	if (find_option_type(key) != NUM) {
		error("unknown numeric variable %s has numeric value", key);
		return;
	}
	if (!o) {
		fprintf(stderr,
			"Bug: Option table filled up trying to set <%s,%u[0x%x]>!\n",
			key,value,value);
		return;
	}
	o->name = strdup(key);
	o->type = NUM;
	o->value.num = value;
}

void
Options::set_flag_option(char *key, bool value)
{
	Option *o = optstruct(key, true);

#ifdef OPTIONS_DEBUG
	if (options_debug)
		fprintf(stderr, "FLAG option <%s,%s>\n",key,value ? "TRUE" : "FALSE");
#endif
	if (find_option_type(key) != FLAG) {
		error("unknown Boolean variable %s has Boolean value", key);
		return;
	}
	if (!o) {
		fprintf(stderr,
			"Bug: Option table filled up trying to set <%s,%s>!\n",
			key,value ? "TRUE" : "FALSE");
		return;
	}
	o->name = strdup(key);
	o->type = FLAG;
	o->value.flag = value;
}


/* Returns NULL if the string pointed to by CRACK_SMOKER
 * does not start with CRACK, or a pointer into CRACK_SMOKER
 * after the prefix if it does.
 */
char *
Options::strprefix(char *crack_smoker, char *crack)
{
	while (*crack_smoker++ == *crack++);
	return (*--crack ? NULL : --crack_smoker);
}

int
Options::find_option_type(char *option)
{
	Option *o;

	for (o = nametable; o->type; o++) {
		if (strcmp(option, o->name) == 0) {
			return o->type;
		}
	}
	return 0;
}

void
Options::process_one_option(char *option)
{
	char *copy = strdup(option), *equals = NULL, *trailer = NULL;
	uint32 num;

#ifdef OPTIONS_DEBUG
	if (options_debug)
		fprintf(stderr, "Got option: <%s>\n",copy);
#endif

	if ((equals = strchr(copy, '=')) == NULL) {
		/* FLAG option */
		if ((trailer = strprefix(copy, "no")) == NULL) {
			/* FLAG set to TRUE */
			set_flag_option(copy, true);
		} else {
			/* FLAG set to FALSE */
			set_flag_option(trailer, false);
		}
	} else {
		/* STR or NUM option */
		*equals++ = '\0';
		switch(find_option_type(copy)) {
			case STR:
				set_str_option(copy, equals);
				break;
			case NUM:
				num = strtoul(equals, NULL, 0);
				set_num_option(copy, num);
				break;
			default:
				*--equals = '=';
				fprintf(stderr, "Unknown option: %s\n", copy);
				break;
		}
	}
	free(copy);
}

/* This could probably be improved upon. Perhaps it should
 * be replaced with a Lex rule-set or something...
 */
int
Options::process_first_option(char **bufptr, int lineno, char *filename)
{
	char *out, *in, copybuf[BUFSIZ], char_seen;
	bool quoting, quotenext, done, string_done;
	
	out = *bufptr;
	in = copybuf;
	done = string_done = quoting = quotenext = false;

	while (isspace(*out)) {
		out++;
	}
	while (!done) {
		char_seen = *out++;
		if (char_seen == '\0') {
			done = true;
			string_done = true;
		} else {
			if (quoting) {
				if (char_seen == '\'') {
					quoting = false;
				} else {
					*in++ = char_seen;
				}
			} else if (quotenext) {
				*in++ = char_seen;
				quotenext = false;
			} else {
				if (char_seen == '\'') {
					quoting = true;
				} else if (char_seen == '\\') {
					quotenext = true;
				} else if (char_seen == '#') {
					done = true;
					string_done = true;
				} else if (!isspace(char_seen)) {
					*in++ = char_seen;
				} else {
					done = true;
					string_done = (out[0] == '\0');
				}
			}
		}
	}
	*in++ = '\0';
	*bufptr = out;
	if (quoting) {
		fprintf(stderr,
			"warning: unterminated quote in config file %s, line %d\n",
			filename, lineno);
	}
	if (strlen(copybuf) > 0) {
		process_one_option(copybuf);
	}
	return string_done ? 0 : 1;
}

int
Options::process_options_from_file(char *filename)
{
	FILE *f;
	char *buf, *p;
	bool done = false;
	int rc, lineno;

	buf = new char[BUFSIZ];
	if (!buf) {
		fatal_error("Can't allocate %u bytes for I/O buffer; aborting!\n",
			BUFSIZ);
	}

	f = fopen(filename, "r");
	if (!f) {
		if (errno != ENOENT) {
			fprintf(stderr, "Can't open config file %s: %s\n",
				filename, strerror(errno));
		}
		return -1;
	}
	p = buf;
	lineno = 1;
	fgets(buf, BUFSIZ, f);
	do {
		do {
			rc = process_first_option(&p, lineno, filename);
		} while (rc == 1);
		p = buf;
		if (fgets(buf, BUFSIZ, f) == NULL) {
			done = true;
		}
		lineno++;
	} while (!done);

	fclose(f);
	delete [] buf;
	return 0;
}

void
Options::usage(char *argv0)
{
	printf(
"Usage: %s [OPTION]... [ROM-FILE]\n"
"Start the %s virtual machine, using the ROM-FILE as the boot ROM.\n"
"\n"
"  -o ARG                     behave as if ARG were specified in .vmipsrc\n"
"                               (see manual for details)\n"
"  -D                         turn on debugging of configuration file parsing\n"
"                               (must be configured at compile time)\n"
"  --version                  display version information and exit\n"
"  --help                     display this help message and exit\n"
"  --print-config             display compile-time variables and exit\n"
"\n"
"By default, `romfile.rom' is used if no ROM-FILE is specified.\n"
"\n"
"Report bugs to <vmips@dgate.org>.\n",
	PACKAGE, PACKAGE);
}

void
Options::process_options_from_args(int argc, char **argv)
{
	bool done = false, error = false;

	while (!done) {
		switch(getopt(argc, argv, "o:")) {
			case 'o':
				process_one_option(optarg);
				break;
			case ':':
			case '?':
				error = true;
				usage(argv[0]);
			case EOF:
			default:
				if (optopt == 'D') {
					/* This option is checked and removed from argv before
					 * we ever got here, if we're doing anything interesting
					 * with it. So we merely print an error message here.
					 */
					fprintf(stderr,
						"Recompile with -DOPTIONS_DEBUG for -D option!\n");
				}
				done = true;
				break;
		}
	}
	if (argc - optind > 1) {
		usage(argv[0]);
		error = true;
	} else if (argc - optind == 1) {
		set_str_option("romfile", argv[optind]);
	}
	if (error) {
		exit(1);
	}
}

void
Options::process_options(int argc, char **argv)
{
	char user_config_filename[PATH_MAX],
		*newargv[ARG_MAX], *extra_args, *argptr;
	int newargc = 0, x, first = 0;

#ifdef OPTIONS_DEBUG
	if (argc > 1 && strcmp(argv[1], "-D") == 0) {
		options_debug = true;
		fprintf(stderr,"WHEEE!! Options debugging turned on!\n");
		argv++;
		argc--;
	}
#endif
	if (argc > 1 && strcmp(argv[1], "--version") == 0) {
		print_package_version();
		exit(0);
	}
	if (argc > 1 && strcmp(argv[1], "--help") == 0) {
		usage(argv[0]);
		exit(0);
	}
	if (argc > 1 && strcmp(argv[1], "--print-config") == 0) {
		print_config_info();
		exit(0);
	}
	/* Get options from defaults. */
	process_defaults();
	/* Get options from system configuration file */
	process_options_from_file(SYSTEM_CONFIG_FILE);
	/* Get options from user configuration file */
	strcpy(user_config_filename, option("configfile")->str);
	tilde_expand(user_config_filename);
	process_options_from_file(user_config_filename);
	/* Get options from command line and VMIPS env var */
	extra_args = getenv("VMIPS");
	if (extra_args != NULL) {
		for (x = 0; x < ARG_MAX; x++) {
			newargv[x] = NULL;
		}
		newargv[newargc++] = argv[0]; /* copy program name or getopt barfs */
		while ((argptr = strtok((first++ == 0) ? extra_args : NULL,
			" \t\r\n")) != NULL) {
			newargv[newargc++] = argptr;
		}
		for (x = 1; x < argc; x++) {
			newargv[newargc++] = argv[x];
		}
		newargv[newargc] = NULL;
		process_options_from_args(newargc, newargv);
	} else {
		process_options_from_args(argc, argv);
	}
#ifdef OPTIONS_DEBUG
	if (options_debug)
		dump_options_table(table);
#endif
}

uint16
Options::hash(char *name)
{
	uint16 i;

	for (i = 0; *name; name++) {
		i += *name;
	}
	return i % 256;
}

Option *
Options::optstruct(char *name, bool install)
{
	int x, y;

	x = hash(name);
	for (y = x; y < TABLESIZE; y++) {
		if (table[y].name != NULL && strcmp(table[y].name, name) == 0)
			return &table[y];
	}
	for (y = 0; y < x; y++) {
		if (table[y].name != NULL && strcmp(table[y].name, name) == 0)
			return &table[y];
	}
	if (install) {
		for (y = x; y < TABLESIZE; y++) {
			if (table[y].name == NULL)
				return &table[y];
		}
		for (y = 0; y < x; y++) {
			if (table[y].name == NULL)
				return &table[y];
		}
	}
	return NULL;
}

union OptionValue *
Options::option(char *name)
{
	Option *o = optstruct(name);

	if (o)
		return &o->value;
	return NULL;
}

#ifdef OPTIONS_DEBUG
void
Options::dump_options_table(Option *table)
{
	int x, nprint = 0;
	char *type[] = { "UNUSED", "FLAG", "NUM", "STR" };

	for (x = 0; x < TABLESIZE; x++) {
		/* don't print UNUSED entries... too much SPAM! */
		if (!table[x].name) continue;

		nprint++;

		fprintf(stderr,"[%d] \"%s\", %s",x,table[x].name ? table[x].name :
			"(NULL)",type[table[x].type]);
		switch(table[x].type) {
			case FLAG:
				fprintf(stderr,", %s",table[x].value.flag ? "TRUE" : "FALSE");
				break;
			case STR:
				fprintf(stderr,", \"%s\"",table[x].value.str);
				break;
			case NUM:
				fprintf(stderr,", 0x%x",table[x].value.num);
				break;
		}
		fputc('\t', stderr);
		if (nprint % 2 == 1 || x == (TABLESIZE - 1)) {
			fputc('\n',stderr);
		}
	}
}
#endif

void
Options::print_config_info(void)
{
	if (WORDS_BIGENDIAN) {
		puts("Host processor big endian");
	} else {
		puts("Host processor little endian");
	}

	if (TARGET_BIG_ENDIAN) {
		puts("Emulated processor big endian");
	} else if (TARGET_LITTLE_ENDIAN) {
		puts("Emulated processor little endian");
	} else {
		puts("Emulated processor endianness unknown");
	}

#ifdef BYTESWAPPED
    puts("Host is byte-swapped with respect to target");
#else
    puts("Host is not byte-swapped with respect to target");
#endif

#ifdef TTY
    puts("SPIM-compatible console device configured");
#else
    puts("SPIM-compatible console device not configured");
#endif

#ifdef INTENTIONAL_CONFUSION
    puts("Registers initialized to random values instead of zero");
#else
    puts("Registers initialized to zero");
#endif

#ifdef OPTIONS_DEBUG
    puts("Options debugging enabled");
#else
    puts("Options debugging disabled");
#endif

#ifdef HAVE_LONG_LONG
    puts("Host compiler has native support for 8-byte integers");
#else
    puts("Host compiler does not natively support 8-byte integers");
#endif
}

void
Options::print_package_version(void)
{
	printf(
"%s %s\n"
"Copyright (C) 2001, 2002, 2003 by Brian R. Gaeke and others.\n"
"(See the files `AUTHORS' and `THANKS' in the %s source distribution\n"
"for a complete list.)\n"
"\n"
"%s is free software; you can redistribute it and/or modify it\n"
"under the terms of the GNU General Public License as published by the\n"
"Free Software Foundation; either version 2 of the License, or (at your\n"
"option) any later version.\n"
"\n"
"%s is distributed in the hope that it will be useful, but\n"
"WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY\n"
"or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License\n"
"for more details.\n"
"\n"
"You should have received a copy of the GNU General Public License along\n"
"with %s; if not, write to the Free Software Foundation, Inc.,\n"
"59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.\n",
	PACKAGE, VERSION, PACKAGE, PACKAGE, PACKAGE, PACKAGE);
}

