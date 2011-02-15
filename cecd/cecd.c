/*
 * cecd - An HDMI-CEC Daemon
 *
 * Copyright (c) 2010, Pete Batard <pbatard@gmail.com>
 * Original daemon skeleton (c) 2001, Levent Karakas
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

static char RUNNING_DIR[] = "/tmp";
static char LOCK_FILE[] = "/var/lock/cecd.lock";
static char LOG_FILE[] = "/var/log/cecd.log";
static char CEC_DEVICE[] = "/dev/cec/0";
static char CONF_FILE[] = "/etc/cecd.conf";

char* running_dir = RUNNING_DIR;
char* lock_file = LOCK_FILE;
char* log_file = LOG_FILE;
char* cec_device = CEC_DEVICE;
char* conf_file = CONF_FILE;

// TODO: pick up from conf
#define OUR_DEVTYPE CEC_DEVTYPE_PLAYBACK
#define BROADCAST (OUR_DEVTYPE<<4 | 0x0F)

static FILE* log_fd = NULL;
static int lock_fd;

static int opt_stdout = 0;
static int opt_interactive = 0;
static int log_level = LIBCEC_LOG_LEVEL_DEBUG;
static int opt_debug = 0;

void cecd_log(const char *format, ...)
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

void signal_handler(int sig)
{
	switch(sig) {
	case SIGHUP:
		cecd_log("hangup signal detected.\n");
		break;
	case SIGTERM:
		cecd_log("terminate signal detected.\n");
		if (!opt_stdout) {
			fclose(log_fd);
		}
		close(lock_fd);
		unlink(lock_file);
		exit(EXIT_SUCCESS);
		break;
	}
}

void daemonize(void)
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

	signal(SIGHUP,signal_handler); /* catch hangup signal */
	signal(SIGTERM,signal_handler); /* catch kill signal */
}

void usage(void)
{
	printf("Usage: cecd [-h|--help] [--usage] [-D|--daemon] [-i|--interactive]\n");
	printf("        [-s|--stdout] [-l|--log-level LOGLEVEL]\n");
	printf("        [-c|--config-file CONFIGFILE] [-d|--device DEVICE]\n");
	printf("        [-t|--type DEVICETYPE] [-v|--version] [--logfile=LOGFILE]\n");
	printf("        [--lockfile=LOCKFILE] [--rundir=RUNNINGDIR]\n");
}

void version(void) {
	printf("Version %d.%d.%d (r%d)\n",
		LIBCEC_VERSION_MAJOR, LIBCEC_VERSION_MINOR, LIBCEC_VERSION_MICRO, LIBCEC_VERSION_NANO);
}

void help(void)
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

int main(int argc, char** argv)
{
	int c, len, ret_val = 0;
	unsigned short physical_address = 0xFFFF;
	unsigned char buffer[128];
	libcec_device_handle* handle;
	long r;
	profile_t profile;

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

	while(1)
	{
		c = getopt_long(argc, argv, "?Dishvl:c:d:", long_options, NULL);
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
		case 'b':
			opt_debug = -1;
			break;
		default:
			printf("am there?");
			help();
			exit(0);
		}
	}

	if (access(conf_file, R_OK) != 0) {
		printf("unable to open conf file '%s'\n", conf_file);
		exit(EXIT_FAILURE);
	}
	r = profile_init_path(conf_file, &profile);
	if (r) {
		fprintf(stderr, "error while initializing profile: %s", profile_errtostr(r));
	}
//	char* test="dump";
//	do_cmd(profile, &test);
	profile_release(profile);

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

	cecd_log("cecd v%d.%d.%d (r%d) started.\n",
		LIBCEC_VERSION_MAJOR, LIBCEC_VERSION_MINOR, LIBCEC_VERSION_MICRO, LIBCEC_VERSION_NANO);

	libcec_set_logging(log_level, log_fd);
	libcec_init();
	if (libcec_open(cec_device, &handle) <0) {
		cecd_log("cannot open CEC device %s\n", cec_device);
		goto out;
	}

	if (libcec_get_physical_address(handle, &physical_address) != LIBCEC_SUCCESS) {
		cecd_log("could not read physical address\n");
	}

	// TODO: pick up from conf
	// Per the CEC spec, logical addresses 4, 8, and 11 are reserved for playback devices
	if (libcec_set_logical_address(handle, 4) < 0) {
		cecd_log("failed to set CEC logical address\n");
		goto out1;
	}

	while(1) {
		len = libcec_read_message(handle, buffer, 128, -1);
		if (len <= 0) {
			cecd_log("Could not read message (error %d)\n", len);
			goto out1;
		}
		libcec_decode_message(buffer, len);
#define XTREAMER_TEST
#ifdef XTREAMER_TEST
		buffer[0] >>= 4;	// Set whoever was talking to us as dest
		buffer[0] |= OUR_DEVTYPE<<4;
		switch(buffer[1]) {
		case CEC_OP_GIVE_OSD_NAME:
			buffer[1] = CEC_OP_SET_OSD_NAME;
			buffer[2] = 'X';
			buffer[3] = 't';
			buffer[4] = 'r';
			buffer[5] = 'e';
			buffer[6] = 'a';
			buffer[7] = 'm';
			buffer[8] = 'e';
			buffer[9] = 'r';
			buffer[10] = ' ';
			buffer[11] = 'P';
			buffer[12] = 'r';
			buffer[13] = 'o';
			len = 14;
			break;
		case CEC_OP_GIVE_DEVICE_VENDOR_ID:
			buffer[0] = BROADCAST;
			buffer[1] = CEC_OP_DEVICE_VENDOR_ID;
			buffer[2] = 0x00;
			buffer[3] = 0x1C;
			buffer[4] = 0x85;	// Eunicorn Korea
			len = 5;
			break;
		case CEC_OP_MENU_REQUEST:
			buffer[1] = CEC_OP_MENU_STATUS;
			buffer[2] = 0x00;	// menu deactivated
			len = 3;
			break;
		case CEC_OP_GIVE_DEVICE_POWER_STATUS:
			buffer[1] = CEC_OP_REPORT_POWER_STATUS;
			buffer[2] = 0x00;	// ON
			len = 3;
			break;
		case CEC_OP_GET_CEC_VERSION:
			buffer[1] = CEC_OP_CEC_VERSION;
			buffer[2] = 0x04;	// version 1.3a
			len = 3;
			break;
		case CEC_OP_GIVE_PHYSICAL_ADDRESS:
			buffer[0] = BROADCAST;
			buffer[1] = CEC_OP_REPORT_PHYSICAL_ADDRESS;
			buffer[2] = physical_address >> 8;
			buffer[3] = physical_address & 0xFF;
			buffer[4] = OUR_DEVTYPE;
			len = 5;
			break;
		case CEC_OP_SET_STREAM_PATH:
			/* Ignore if request is for a different phys_addr */
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
			buffer[2] = 0x11;	// "Play"
			len = 3;
			break;
		default:
			len = 0;
			break;
		}
		if (len) {
			if (libcec_write_message(handle, buffer, len)) {
				cecd_log("Could not send message\n");
				goto out1;
			}
			libcec_decode_message(buffer, len);
		}
#endif
	}

out1:
	libcec_close(handle);
out:
	libcec_exit();
	close(lock_fd);
	unlink(lock_file);
	cecd_log("cecd stopped.\n");
	if (!opt_stdout) {
		fclose(log_fd);
	}
	exit(ret_val);
}
