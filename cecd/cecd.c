/*
 * cecd - An HDMI-CEC Daemon
 *
 * Copyright (c) 2010-2011, Pete Batard <pete@akeo.ie>
 * Original daemon skeleton (c) 2001, Levent Karakas
 * CEC code translation inspired by irfake (c) 2010 Sekator500
 * Hash table functions adapted from glibc, (c) Ulrich Drepper et al.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <getopt.h>

#include "libcec.h"
#include "decoder.h"
#include "libcec_version.h"
#include "profile.h"
#include "profile_helpers.h"

#define BROADCAST (device_type<<4 | 0x0F)
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

static char RUNNING_DIR[] = "/tmp";
static char LOCK_FILE[] = "/var/lock/cecd.lock";
static char LOG_FILE[] = "/var/log/cecd.log";
static char CEC_DEVICE[] = "/dev/cec/0";
static char CONF_FILE[] = "/etc/cecd.conf";
static char DEFAULT_DEVICE_NAME[] = "Unidentified";

char* running_dir = RUNNING_DIR;
char* lock_file = LOCK_FILE;
char* log_file = LOG_FILE;
char* cec_device = CEC_DEVICE;
char* conf_file = CONF_FILE;
unsigned int device_type = CEC_DEVTYPE_PLAYBACK;

static FILE *log_fd = NULL, *target_fd = NULL;
static int lock_fd;

static int opt_stdout = 0;
static int opt_interactive = 0;
static int log_level = LIBCEC_LOG_LEVEL_DEBUG;
static int opt_debug = 0;

static profile_t profile;
static libcec_device_handle* handle;

/* command translation */
static int target_packet_size, target_repeat;
typedef struct seq {
	uint8_t  len;
	uint16_t* data;		// hash values (for CEC commands) or simple bytes (UI codes)
	char* trans;
	struct seq* next;	// chained list
} seq;
static seq **seq_ucp, **seq_cec;
static char **key_list_ucp = NULL, **key_list_cec = NULL;

static void cecd_log(const char *format, ...)
{
	va_list args;
	struct timeval tv;
	struct tm *loc;

	gettimeofday(&tv, (struct timezone *)0);
	loc = localtime(&tv.tv_sec);
	fprintf(log_fd, "%04d.%02d.%02d %02d:%02d:%02d.%03ld ",
		loc->tm_year+1900, loc->tm_mon+1, loc->tm_mday, loc->tm_hour,
		loc->tm_min, loc->tm_sec, tv.tv_usec/1000);
	va_start(args, format);
	vfprintf(log_fd, format, args);
	va_end(args);
	fflush(log_fd);
}
#define cecd_dbg(...) do { if (opt_debug) cecd_log(__VA_ARGS__); } while(0)

static void daemonize(void)
{
	pid_t pid, sid;
	int fd, ignored;
	char str[10];

	if (getppid() == 1) {
		// already a daemon
		return;
	}
	pid = fork();
	if (pid < 0) {
		// fork error
		exit(EXIT_FAILURE);
	}
	if (pid > 0) {
		// parent exit
		_exit(EXIT_SUCCESS);
	}

	// child (daemon) continues
	sid = setsid();
	if (sid < 0) {
		exit(EXIT_FAILURE);
	}

	if (!opt_stdout) {
		// disable all stdio
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
		fd = open("/dev/null", O_RDWR);
		ignored = dup(fd);
		ignored = dup(fd);
	}

	umask(027);
	ignored = chdir(running_dir);
	lock_fd = open(lock_file, O_RDWR|O_CREAT, 0640);
	if (lock_fd < 0) {
		exit(EXIT_FAILURE);
	}
	if (lockf(lock_fd, F_TLOCK, 0) < 0) {
		// another daemon is already running
		exit(EXIT_SUCCESS);
	}

	if (!opt_stdout) {
		log_fd = fopen(log_file, "a");
	} else {
		log_fd = stdout;
	}

	// first daemon instance continues
	if (sprintf(str, "%d\n", getpid()) < 0) {
		exit(EXIT_FAILURE);
	}
	if (write(lock_fd, str, strlen(str)) != strlen(str)) {
		exit(EXIT_FAILURE);
	}
	signal(SIGCHLD,SIG_IGN);
	signal(SIGTSTP,SIG_IGN);
	signal(SIGTTOU,SIG_IGN);
	signal(SIGTTIN,SIG_IGN);
}

/* Insert a new seq element at the start of the chain */
static int seq_add(seq** list_start, uint16_t* data, uint8_t len, char* trans)
{
	seq* new_seq = malloc(sizeof(seq));
	if (new_seq == NULL) {
		return -1;
	}
	new_seq->data = malloc(len*sizeof(uint16_t));
	if (new_seq->data == NULL) {
		free(new_seq);
		return -1;
	}
	memcpy(new_seq->data, data, len*sizeof(uint16_t));
	new_seq->len = len;
	new_seq->trans = trans;
	new_seq->next = *list_start;

	*list_start = new_seq;
	return 0;
}

static void seq_table_free(seq** table, uint32_t size)
{
	seq *cur, *p;
	uint32_t i;

	if (table == NULL)
		return;

	for (i=0; i<size; i++) {
		cur = table[i];
		while (cur != NULL) {
			p = cur;
			free(p->data);
			cur = p->next;
			free(p);
		}
	}
	free(table);
}

/*
 * Hash table functions (from glibc 2.3.2, modified)
 */
typedef struct entry {
	uint32_t used;
	uint8_t* data;
	uint8_t  len;
} entry;

typedef struct hash_table {
	const char* name;
	entry* table;
	uint32_t size;
	uint32_t filled;
} hash_table;

static hash_table *htab_cec = NULL;

/* For the used double hash method the table size has to be a prime. To
   correct the user given table size we need a prime test.  This trivial
   algorithm is adequate because the code is called only during init and
   the number is likely to be small  */
static int isprime(uint32_t number)
{
	// no even number will be passed
	uint32_t divider = 3;

	while((divider * divider < number) && (number % divider != 0))
		divider += 2;

	return (number % divider != 0);
}

/* Before using the hash table we must allocate memory for it.
   We allocate one element more as the found prime number says.
   This is done for more effective indexing as explained in the
   comment for the hash function.  */
static hash_table* htab_create(uint32_t nel, const char* name)
{
	hash_table* htab = calloc(1, sizeof(hash_table));
	if (htab == NULL) return NULL;

	// change nel to the first prime number not smaller as nel.
	nel |= 1;
	while(!isprime(nel))
		nel += 2;

	// constrain to 65534 elements so that the hash fits an uint16_t
	if (nel > 0x10000) {
		free(htab);
		return NULL;
	}

	htab->size = nel;
	htab->name = name;
	cecd_log("using %d entries hash table for %s\n", nel, name);
	htab->filled = 0;

	// allocate memory and zero out.
	htab->table = (entry*)calloc(htab->size + 1, sizeof(entry));
	if (htab->table == NULL) {
		free(htab);
		return NULL;
	}
	return htab;
}

/* After using the hash table it has to be destroyed.  */
static void htab_free(hash_table* htab)
{
	size_t i;

	if (htab == NULL)
		return;

	if (htab->table != NULL) {
		for (i=0; i<htab->size; i++) {
			if (htab->table[i].used) {
				free(htab->table[i].data);
			}
		}
		free(htab->table);
	}
	free(htab);
}

/* This is the search function. It uses double hashing with open addressing.
   We use an trick to speed up the lookup. The table is created with one
   more element available. This enables us to use the index zero special.
   This index will never be used because we store the first hash index in
   the field used where zero means not used. Every other value means used.
   The used field can be used as a first fast comparison for equality of
   the stored and the parameter value. This helps to prevent unnecessary
   expensive calls of memcmp.  */
static uint16_t htab_hash(uint8_t* data, uint8_t len, hash_table* htab, int create)
{
	uint32_t hval, hval2;
	uint32_t idx;
	uint32_t r = 5381;
	uint8_t i;

	if ((htab == NULL) || (htab->table == NULL) || (data == NULL) || (len == 0)) {
		return 0;
	}

	// Compute main hash value
	for (i=0; i<len; i++)
		r = ((r << 5) + r) + data[i];
	if (r == 0)
		++r;

	// compute table hash: simply take the modulus
	hval = r % htab->size;
	if (hval == 0)
		++hval;

	// Try the first index
	idx = hval;

	if (htab->table[idx].used) {
		if ( (htab->table[idx].used == hval)
		  && (htab->table[idx].len == len)
		  && (memcmp(data, htab->table[idx].data, len) == 0) ) {
			// existing hash
			return idx;
		}
		cecd_dbg("hash collision detected for table %s\n", htab->name);

		// Second hash function, as suggested in [Knuth]
		hval2 = 1 + hval % (htab->size - 2);

		do {
			// Because size is prime this guarantees to step through all available indexes
			if (idx <= hval2) {
				idx = htab->size + idx - hval2;
			} else {
				idx -= hval2;
			}

			// If we visited all entries leave the loop unsuccessfully
			if (idx == hval) {
				break;
			}

			// If entry is found use it.
			if ( (htab->table[idx].used == hval)
			  && (htab->table[idx].len == len)
			  && (memcmp(data, htab->table[idx].data, len) == 0) ) {
				return idx;
			}
		}
		while (htab->table[idx].used);
	}

	// Not found => New entry
	if (!create)
		return 0;

	// If the table is full return an error
	if (htab->filled >= htab->size) {
		cecd_log("hash table %s is full (%d entries)\n", htab->name, htab->size);
		return 0;
	}

	// free any previously allocated data (in case of duplicate data entries)
	free(htab->table[idx].data);
	htab->table[idx].used = hval;
	htab->table[idx].len = len;
	htab->table[idx].data = malloc(len);
	if (htab->table[idx].data == NULL) {
		cecd_log("could not duplicate data for hash table %s\n", htab->name);
		return 0;
	}
	memcpy(htab->table[idx].data, data, len);
	++htab->filled;

	cecd_dbg("created key 0x%lx\n", idx);
	// the table was constrained to less than 0x10000 elements during creation
	return (uint16_t)idx;
}

static void usage(void)
{
	printf("Usage: cecd [-h|--help] [--usage] [-D|--daemon] [-i|--interactive]\n");
	printf("        [-s|--stdout] [-l|--log-level LOGLEVEL]\n");
	printf("        [-c|--config-file CONFIGFILE] [-d|--device DEVICE]\n");
	printf("        [-t|--type DEVICETYPE] [-v|--version] [--logfile=LOGFILE]\n");
	printf("        [--lockfile=LOCKFILE] [--rundir=RUNNINGDIR]\n");
}

static void version(void) {
	printf("Version %d.%d.%d (r%d)\n",
		LIBCEC_VERSION_MAJOR, LIBCEC_VERSION_MINOR, LIBCEC_VERSION_MICRO, LIBCEC_VERSION_NANO);
}

static void help(void)
{
	printf("Usage: cecd [OPTION...]\n");
	printf("  -D, --daemon                            Become a daemon (default)\n");
	printf("  -i, --interactive                       Run interactive (not a daemon)\n");
	printf("  -s, --stdout                            Log to stdout\n");
	printf("\n");
	printf("Help options:\n");
	printf("  -h, --help                              Show this help message\n");
	printf("  --usage                                 Display brief usage message\n");
	printf("\n");
	printf("Common options:\n");
	printf("  -l, --log-level=LOGLEVEL                Set logging level\n");
	printf("  -c, --config-file=CONFIGFILE            Use alternate configuration file\n");
	printf("  -d, --device=DEVICE                     Use alternate CEC device\n");
	printf("  -t, --type=DEVICETYPE                   Set CEC device logical type\n");
	printf("  -v, --version                           Print version\n");
	printf("\n");
	printf("Build-time configuration overrides:\n");
	printf("  --logfile=LOGFILE                       Use alternate log file\n");
	printf("  --lockfile=LOCKFILE                     Use alternate lock file\n");
	printf("  --rundir=RUNNINGDIR                     Path to running directory\n");
}

static int str_to_byte(char* str, uint8_t* byte)
{
	uint32_t val;
	char *end_str;

	if ((str == NULL) || (str[0] == 0)) {
		return -1;
	}

	if ((str[0] == '0') && (str[1] == 'x')) {
		val = strtoul(str+2, &end_str, 16);
	} else {
		val = strtoul(str, &end_str, 10);
	}

	if ((val > 0xFF) || (end_str != str + strlen(str))) {
		return -1;
	}

	*byte = (uint8_t)val;
	return 0;
}

static int str_to_uint32(char* str, uint32_t* val)
{
	char *end_str;

	if ((str == NULL) || (str[0] == 0)) {
		return -1;
	}

	if ((str[0] == '0') && (str[1] == 'x')) {
		*val = strtoul(str+2, &end_str, 16);
	} else {
		*val = strtoul(str, &end_str, 10);
	}

	if ( ((*val == ULONG_MAX) && errno != 0)
	  || (end_str != str + strlen(str)) ) {
		return -1;
	}

	return 0;
}

static uint16_t cmdstr_to_hash(char* cmdstr, hash_table *htab) {

	char *str, *saveptr;
	uint8_t byte, cmd_len, cmd_data[CEC_MAX_COMMAND_SIZE];

	str = strtok_r(cmdstr, ",", &saveptr);
	for (cmd_len=0; ((str!=NULL)&&(cmd_len<ARRAY_SIZE(cmd_data))); cmd_len++) {
		if (str_to_byte(str, &byte)) {
			cmd_len = 0;
			break;
		}
		cmd_data[cmd_len] = byte;
		str = strtok_r(NULL, ",", &saveptr);
	}

	return htab_hash(cmd_data, cmd_len, htab, 1);
}

static void cmd_execute(char* command) {
	uint32_t packet;

	// TODO: handle target sequences and stuff
	if (str_to_uint32(command, &packet)) {
		cecd_log("execute: '%s' [NOT IMPLEMENTED YET]\n", command);
		return;
	}

	if (target_fd != NULL) {
		fwrite(&packet, target_packet_size, 1, target_fd);
		if (target_repeat) {
			fwrite(&packet, target_packet_size, 1, target_fd);
		}
		fflush(target_fd);
		cecd_dbg("execute: sent packet 0x%08x\n", packet);
	}
}

/* process and execute a sequence of UI commands */
static uint8_t cmd_process(seq** seq_table, uint16_t* buffer, uint8_t len, uint8_t max_len)
{
	seq* cur;
	seq *last_match = NULL;
	int match = 0;
	uint8_t processed_len, right_len;

	cur = seq_table[buffer[0]];
	// Lookup all possible matches with a length greater or equal to the current
	while (cur != NULL) {
		if ( (cur->len >= len) && (cur->len <= max_len)
		  && (memcmp(cur->data, buffer, MIN(cur->len, len)*sizeof(uint16_t)) == 0) ) {
			match++;
			last_match = cur;
		}
		cur = cur->next;
	}

	if (match == 0) {
		// if it's a single item, then it's unhandled => eliminate it
		if (len == 1) {
			cecd_dbg("process: unhandled single item 0x%04x: ignoring.\n", buffer[0]);
			return 1;
		}
		cecd_dbg("process: no match/broken sequence: recursing\n");
		// broken sequence => go back and recursively try to match the previous subsequences
		// while eliminating potential matches longer than len-1 this time around
		processed_len = cmd_process(seq_table, buffer, len-1, len-1);
		// There's no limit on potential matches for the second part of our lookup
		right_len = cmd_process(seq_table, &buffer[processed_len], len-processed_len, 0xFF);
		return processed_len + right_len;
	}

	if (match == 1) {
		if (last_match->len == len) {
			// if the lengths are the same, we have a single match
			cmd_execute(last_match->trans);
			return len;
		}
		// last_match->len > len => need a few more items to declare victory
		return 0;
	}

	// match > 1 => multiple possible matches
	return 0;
}

static void cecd_exit(int ret_val)
{
	// All these calls properly handle a NULL parameter
	seq_table_free(seq_ucp, 256);
	profile_free_list(key_list_ucp);
	if (htab_cec != NULL) {
		seq_table_free(seq_cec, htab_cec->size);
	}
	profile_free_list(key_list_cec);
	htab_free(htab_cec);
	libcec_close(handle);
	fclose(target_fd);
	profile_release(profile);
	libcec_exit();
	close(lock_fd);
	unlink(lock_file);
	cecd_log("cecd stopped.\n");
	if (!opt_stdout) {
		fclose(log_fd);
	}
	exit(ret_val);
}

static void signal_handler(int sig)
{
	switch(sig) {
	case SIGHUP:
		cecd_log("hangup signal detected.\n");
		if (opt_interactive)
			cecd_exit(EXIT_SUCCESS);
		break;
	case SIGTERM:
		cecd_log("terminate signal detected.\n");
		cecd_exit(EXIT_SUCCESS);
		break;
	}
}

int main(int argc, char** argv)
{
	const char* ucp_commands_node[3] = {"translate", "ucp_commands", 0};
	const char* cec_commands_node[3] = {"translate", "cec_commands", 0};
	long r;
	int c, len, target_timeout, logical_address = 15, physical_address_changed = -1;
	uint32_t size, device_oui;
	uint16_t physical_address = 0xFFFF;
	// TODO: check for seq_data overflow
	uint16_t seq_data[CEC_MAX_COMMAND_SIZE], seq_len, ucp_unprocessed[CEC_MAX_COMMAND_SIZE], cec_unprocessed[CEC_MAX_COMMAND_SIZE];
	uint8_t i, byte, buffer[CEC_MAX_COMMAND_SIZE];
	uint8_t ucp_unprocessed_len = 0, ucp_processed_len, cec_unprocessed_len = 0, cec_processed_len;
	char *target_device, *device_name, *str, *saveptr, **key, *val;

	static struct option long_options[] = {
		{"daemon", no_argument, 0, 'D'},
		{"interactive", no_argument, 0, 'i'},
		{"stdout", no_argument, 0, 's'},
		{"help", no_argument, 0, 'h'},
		{"usage", no_argument, 0, 1000},
		{"version", no_argument, 0, 'v'},
		{"log-level", required_argument, 0, 'l'},
		{"config-file", required_argument, 0, 'c'},
		{"device", required_argument, 0, 'd'},
		{"logfile", required_argument, 0, 1001},
		{"lockfile", required_argument, 0, 1002},
		{"rundir", required_argument, 0, 1003},
		{0, 0, 0, 0}
	};

#ifndef PROFILE_DEBUG
	while(1)
	{
		c = getopt_long(argc, argv, "?Dishvbl:c:d:", long_options, NULL);
		if (c == -1)
			break;
		switch(c) {
		case 1000:
			usage();
			exit(0);
		case 1001:
			log_file = optarg;
			break;
		case 1002:
			lock_file = optarg;
			break;
		case 1003:
			running_dir = optarg;
			break;
		case 'D':
			opt_interactive = 0;
			break;
		case 'i':
			opt_interactive = -1;
			break;
		case 's':
			opt_stdout = -1;
			break;
		case 'c':
			conf_file = optarg;
			break;
		case 'd':
			cec_device = optarg;
			break;
		case 'l':
			log_level = (int)strtol(optarg, NULL, 0);
			break;
		case 'h':
			help();
			exit(0);
		case 'v':
			version();
			exit(0);
		case 'b':	// Development debug
			opt_debug = -1;
			break;
		default:
			help();
			exit(EXIT_SUCCESS);
		}
	}

	/*
	 * Start the daemon if requested
	 */
	if (!opt_interactive) {
		daemonize();
	} else {
		if (!opt_stdout) {
			log_fd = fopen(log_file, "a");
			if (!log_fd) {
				exit(EXIT_FAILURE);
			}
		} else {
			log_fd = stdout;
		}
	}
#endif

	if (access(conf_file, R_OK) != 0) {
		printf("unable to open conf file '%s'\n", conf_file);
		cecd_exit(EXIT_FAILURE);
	}
	r = profile_init_path(conf_file, &profile);
	if (r) {
		fprintf(stderr, "error while processing '%s': %s\n",
			conf_file, profile_errtostr(r));
		cecd_exit(EXIT_FAILURE);
	}

#ifdef PROFILE_DEBUG
	do_cmd(profile, argv+1);
	cecd_exit(EXIT_SUCCESS);
#endif

	/*
	 * read the conf file data
	 */
	if ( (profile_get_uint(profile, "device", "type", NULL, CEC_DEVTYPE_PLAYBACK, &device_type))
	  || (device_type > 5) || (device_type == 2) ) {
		cecd_log("invalid value for device.type\n");
		cecd_exit(EXIT_FAILURE);
	}
	if ((r = profile_get_string(profile, "device", "path", NULL, CEC_DEVICE, &cec_device))) {
		cecd_log("error reading device.path: %s\n", profile_errtostr(r));
		cecd_exit(EXIT_FAILURE);
	}
	if ((r = profile_get_string(profile, "device", "name", NULL, DEFAULT_DEVICE_NAME, &device_name))) {
		cecd_log("error reading device.name: %s\n", profile_errtostr(r));
		cecd_exit(EXIT_FAILURE);
	}
	if ((device_name == NULL) || (strlen(device_name) < 1) || (strlen(device_name) > 14)) {
		cecd_log("invalid device.name: '%s' - ignored\n", device_name);
		device_name = DEFAULT_DEVICE_NAME;
	}
	if ((r = profile_get_uint(profile, "device", "oui", NULL, 0xFFFFFF, &device_oui))) {
		cecd_log("error reading device.oui: %s\n", profile_errtostr(r));
		cecd_exit(EXIT_FAILURE);
	}
	if ((r = profile_get_string(profile, "translate", "target", "path", NULL, &target_device))) {
		cecd_log("error reading translate.target.path: %s\n", profile_errtostr(r));
		cecd_exit(EXIT_FAILURE);
	}
	if ( (profile_get_integer(profile, "translate", "target", "packet_size", 4, &target_packet_size))
		|| (target_packet_size > 4) ) {
		cecd_log("invalid value for translate.target.packet_size\n");
		cecd_exit(EXIT_FAILURE);
	}
	if ((r = profile_get_boolean(profile, "translate", "target", "repeat", 0, &target_repeat))) {
		cecd_log("error reading translate.traget.repeat: %s\n", profile_errtostr(r));
		cecd_exit(EXIT_FAILURE);
	};
	if ((r = profile_get_integer(profile, "translate", "target", "timeout", 2000, &target_timeout))) {
		cecd_log("error reading translate.target.timeout: %s\n", profile_errtostr(r));
		cecd_exit(EXIT_FAILURE);
	};

	cecd_log("cecd v%d.%d.%d (r%d) started.\n",
		LIBCEC_VERSION_MAJOR, LIBCEC_VERSION_MINOR, LIBCEC_VERSION_MICRO, LIBCEC_VERSION_NANO);

	libcec_set_logging(log_level, log_fd);
	libcec_init();
	if (libcec_open(cec_device, &handle) <0) {
		cecd_log("cannot open CEC device %s\n", cec_device);
		cecd_exit(EXIT_FAILURE);
	}

	/*
	 * Process the translation sequences
	 */

	// Get the list of all ucp_commands keys
	if ((r = profile_get_relation_names(profile, ucp_commands_node, &key_list_ucp))) {
		cecd_log("error reading ucp commands: %s - ucp table will be ignored\n", profile_errtostr(r));
	} else {
		// allocate the ucp_commands sequence lookup table
		seq_ucp = calloc(256, sizeof(seq*));
		if (seq_ucp == NULL) {
			cecd_log("out of memory (seq_ucp) - aborting\n");
			cecd_exit(EXIT_FAILURE);
		}
		for (key=key_list_ucp; *key != NULL; key++) {
			// Get the value, before we lose the key
			if (profile_get_string(profile, "translate", "ucp_commands", *key, NULL, &val) != 0) {
				cecd_log("unable to read value for ucp_commands key '%s' - ignoring sequence\n", *key);
				continue;
			}
			str = strtok_r(*key, ",", &saveptr);
			// fill up a byte array with the sequence
			for (seq_len=0; ((str!=NULL)&&(seq_len<ARRAY_SIZE(seq_data))); seq_len++) {
				if (str_to_byte(str, &byte)) {
					cecd_log("error converting byte '%s' - ignoring sequence\n", str);
					seq_len = 0;
					break;
				}
				seq_data[seq_len] = (uint16_t)byte;
				str = strtok_r(NULL, ",", &saveptr);
			}
			// store the sequence in a table of chained lists, using data[0] as index
			if ((seq_len > 0) && (seq_add(&seq_ucp[seq_data[0]], seq_data, seq_len, val) != 0)) {
				cecd_log("out of memory (seq_ucp add) - aborting");
				cecd_exit(EXIT_FAILURE);
			}
		}
	}

	// Get the list of all cec_codes sequences
	if ((r = profile_get_relation_names(profile, cec_commands_node, &key_list_cec))) {
		cecd_log("error reading cec commands: %s - cec table will be ignored\n", profile_errtostr(r));
	} else {
		// Find the size
		size = 0;
		for (key=key_list_cec; *key != NULL; key++) {
			size++;
		}
		// Create a hash table that's at least 4 times the size
		// TODO: allow customization of multiplier through cecd opt or conf
		htab_cec = htab_create(size*4, "cec_commands");
		if (htab_cec == NULL) {
			cecd_log("out of memory (htab_cec) - aborting\n");
			cecd_exit(EXIT_FAILURE);
		}
		// Create the sequence array. This is a table of chained list, with each element
		// of the list representing a potential matching sequence
		seq_cec = calloc(htab_cec->size, sizeof(seq*));
		if (seq_cec == NULL) {
			cecd_log("out of memory (seq_cec) - aborting\n");
			cecd_exit(EXIT_FAILURE);
		}
		for (key=key_list_cec; *key != NULL; key++) {
			// Get the value, before we lose the key through strtok
			if (profile_get_string(profile, "translate", "cec_commands", *key, NULL, &val) != 0) {
				cecd_log("unable to read value for cec_commands key '%s' - ignoring sequence\n", *key);
				continue;
			}
			str = strtok_r(*key, ":", &saveptr);
			// fill up a hash array with the sequence
			for (seq_len=0; ((str!=NULL)&&(seq_len<ARRAY_SIZE(seq_data))); seq_len++) {
				seq_data[seq_len] = cmdstr_to_hash(str, htab_cec);
				if (seq_data[seq_len] == 0) {
					cecd_log("error creating hash for command containing '%s' - ignoring sequence\n", str);
					seq_len = 0;
					break;
				}
				str = strtok_r(NULL, ":", &saveptr);
			}
			// store the sequence in a table of chained lists, using data[0] as index
			if ((seq_len > 0) && (seq_add(&seq_cec[seq_data[0]], seq_data, seq_len, val) != 0)) {
				cecd_log("out of memory (seq_cec add) - aborting");
				cecd_exit(EXIT_FAILURE);
			}
		}
	}

	// Open translation target, if provided
	if (target_device != NULL) {
		target_fd = fopen(target_device, "w");
		if (target_fd == NULL) {
			cecd_log("unable to open UI codes translation target '%s'\n", target_device);
			cecd_log("translation of HDMI-CEC codes will be disabled\n");
		} else {
			cecd_log("will use target '%s' for UI codes translation\n", target_device);
		}
	}

	// handle interrupt signals before we enter main loop
	signal(SIGHUP, signal_handler);
	signal(SIGTERM, signal_handler);

	// TODO: handle physical address loss (re-routing)
	while(1) {
		if (physical_address_changed) {
			logical_address = libcec_allocate_logical_address(handle, device_type, &physical_address);
			if (logical_address < 0) {
				cecd_log("failed to set logical address: %s\n", libcec_strerror(logical_address));
				cecd_exit(EXIT_FAILURE);
			}
			cecd_log("logical address set to %d\n", logical_address);
			physical_address_changed = 0;
		}
		// TODO: don't use timeout if no target
		len = libcec_read_message(handle, buffer, ARRAY_SIZE(buffer), target_timeout);
		if (len == LIBCEC_ERROR_TIMEOUT) {
			if ((ucp_unprocessed_len == 0) && (cec_unprocessed_len == 0)) {
				continue;
			}
			// If we have unfinished sequence business, insert a fake
			// <User Control Pressed> message with an invalid key
			cecd_dbg("timeout detected while looking for a sequence - inserting fake message\n");
			buffer[0] = 0xF0 | logical_address;
			buffer[1] = CEC_OP_USER_CONTROL_PRESSED;
			buffer[2] = 0xFF;
			len = 3;
		}
		if (len < 0) {
			cecd_log("could not read message (error %d)\n", len);
			continue;
		}
		libcec_decode_message(buffer, len);
		if (len <= 1) {
			// Ignore ACK, etc.
			continue;
		}

		buffer[0] >>= 4;	// Set whoever was talking to us as dest
		buffer[0] |= logical_address << 4;
		switch(buffer[1]) {
		case CEC_OP_GIVE_OSD_NAME:
			buffer[1] = CEC_OP_SET_OSD_NAME;
			for (i=0; i<strlen(device_name); i++) {
				buffer[i+2] = device_name[i];
			}
			len = i+2;
			break;
		case CEC_OP_GIVE_DEVICE_VENDOR_ID:
			buffer[0] = BROADCAST;
			buffer[1] = CEC_OP_DEVICE_VENDOR_ID;
			buffer[2] = (device_oui>>16)&0xFF;
			buffer[3] = (device_oui>>8)&0xFF;
			buffer[4] = device_oui & 0xFF;
			len = 5;
			break;
		case CEC_OP_MENU_REQUEST:
			buffer[1] = CEC_OP_MENU_STATUS;
			buffer[2] = CEC_MENUSTATE_ACTIVATED;
			len = 3;
			break;
		case CEC_OP_GIVE_DEVICE_POWER_STATUS:
			buffer[1] = CEC_OP_REPORT_POWER_STATUS;
			buffer[2] = CEC_POWERSTATUS_ON;
			len = 3;
			break;
		case CEC_OP_GET_CEC_VERSION:
			buffer[1] = CEC_OP_CEC_VERSION;
			buffer[2] = CEC_VERSION_V1_3A;
			len = 3;
			break;
		case CEC_OP_GIVE_PHYSICAL_ADDRESS:
			buffer[0] = BROADCAST;
			buffer[1] = CEC_OP_REPORT_PHYSICAL_ADDRESS;
			buffer[2] = physical_address >> 8;
			buffer[3] = physical_address & 0xFF;
			buffer[4] = device_type;
			len = 5;
			break;
		case CEC_OP_SET_STREAM_PATH:
			// Ignore if request is for a different phys_addr
			if ((buffer[2] != (physical_address >> 8)) || (buffer[3] != (physical_address & 0xFF)))
				break;
			buffer[0] = BROADCAST;
			buffer[1] = CEC_OP_ACTIVE_SOURCE;
			buffer[2] = physical_address >> 8;
			buffer[3] = physical_address & 0xFF;
			len = 4;
			break;
		case CEC_OP_GIVE_DECK_STATUS:
			buffer[1] = CEC_OP_DECK_STATUS;
			buffer[2] = CEC_DECKINFO_PLAY;
			len = 3;
			break;
		case CEC_OP_USER_CONTROL_PRESSED:
			ucp_unprocessed[ucp_unprocessed_len++] = buffer[2];
			ucp_processed_len = cmd_process(seq_ucp, ucp_unprocessed, ucp_unprocessed_len, 0xFF);
			ucp_unprocessed_len -= ucp_processed_len;
			if (ucp_processed_len && ucp_unprocessed_len) {
				memmove(ucp_unprocessed, ucp_unprocessed+ucp_processed_len, ucp_unprocessed_len*sizeof(uint16_t));
			}
			len = 0;
			break;
		default:
			// Convert to hash, to match against a conf file command
			cec_unprocessed[cec_unprocessed_len++] = htab_hash(buffer+1, len-1, htab_cec, 0);
			cec_processed_len = cmd_process(seq_cec, cec_unprocessed, cec_unprocessed_len, 0xFF);
			cec_unprocessed_len -= cec_processed_len;
			if (cec_processed_len && cec_unprocessed_len) {
				memmove(cec_unprocessed, cec_unprocessed+cec_processed_len, cec_unprocessed_len*sizeof(uint16_t));
			}
			len = 0;
			break;
		}
		if (len) {
			if (libcec_write_message(handle, buffer, len)) {
				cecd_log("could not send message\n");
				continue;
			}
			libcec_decode_message(buffer, len);
		}
	}
	return EXIT_SUCCESS;
}
