#ifndef __options_h__
#define __options_h__

#include "sysinclude.h"

#ifndef CONFIG_PREFIX
#define CONFIG_PREFIX "/usr/local"
#endif /* CONFIG_PREFIX */

#define SYSTEM_CONFIG_FILE CONFIG_PREFIX"/etc/vmipsrc"

/* This is currently only used in the self tests. */
#define NUM_MEMORY_MODULES 16

/* reset vector minus KSEG1_CONST_TRANSLATION */
#define ROM_BASE_ADDRESS 0x1fc00000

#define DUMP_FILE_NAME "memdump.bin" /* eventually this should be an option */

/* option types */
enum {
	FLAG = 1,
	NUM,
	STR
};

union OptionValue {
	char *str;
	bool flag;
	uint32 num;
};

typedef struct {
	char *name;
	int type;
	union OptionValue value;
} Option; 

/* The total number of options must not be greater than TABLESIZE, and
 * TABLESIZE should be the smallest power of 2 greater than the number
 * of options that allows the hash function to perform well.
 */
#define TABLESIZE 256

class Options {
private:
	Option table[TABLESIZE];
	uint16 hash(char *name);
#ifdef OPTIONS_DEBUG
	bool options_debug;
#endif

private:
	void process_options(int argc, char **argv);
	void process_defaults(void);
	void process_one_option(char *option);
	int process_first_option(char **bufptr, int lineno = 0, char *fn = NULL);
	int process_options_from_file(char *filename);
	void process_options_from_args(int argc, char **argv);
	int tilde_expand(char *filename);
	void usage(char *argv0);
	void set_str_option(char *key, char *value);
	void set_num_option(char *key, uint32 value);
	void set_flag_option(char *key, bool value);
	char *strprefix(char *crack_smoker, char *crack);
	int find_option_type(char *option);
	Option *optstruct(char *name, bool install = false);
#ifdef OPTIONS_DEBUG
	void dump_options_table(Option *table);
#endif
public:
	Options(int argc, char **argv);
	union OptionValue *option(char *name);
};

#endif /* __options_h__ */
