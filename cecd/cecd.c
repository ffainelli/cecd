/*
 * cecd - An HDMI-CEC Daemon
 *
 * Copyright (c) 2010, Pete Batard <pbatard@gmail.com>
 * Original daemon skeleton (c) 2001, Levent Karakas
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include "libcec.h"

#define RUNNING_DIR	"/tmp"
#define LOCK_FILE	"/var/lock/cecd.lock"
#define LOG_FILE	"/var/log/cecd.log"
#define CEC_DEVICE	"/dev/cec/0"

static FILE* logfile = NULL;
static int lock_fd;

void signal_handler(int sig)
{
	switch(sig) {
	case SIGHUP:
		fprintf(logfile, "hangup signal detected.\n");
		fflush(logfile);
		break;
	case SIGTERM:
		fprintf(logfile, "terminate signal detected.\n");
		fclose(logfile);
		close(lock_fd);
		unlink(LOCK_FILE);
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

	// disable all stdio
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	fd = open("/dev/null", O_RDWR);
	ignored = dup(fd);
	ignored = dup(fd);

	umask(027);
	ignored = chdir(RUNNING_DIR);
	lock_fd = open(LOCK_FILE, O_RDWR|O_CREAT, 0640);
	if (lock_fd < 0) {
		exit(EXIT_FAILURE);
	}
	if (lockf(lock_fd, F_TLOCK, 0) < 0) {
		// another daemon is already running
		exit(EXIT_SUCCESS);
	}

	logfile = fopen(LOG_FILE, "a");

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

int main(int argc, char** argv)
{
	int len, ret_val = 0;
	unsigned char buffer[128];
	libcec_device_handle* handle;

	daemonize();

	logfile = fopen(LOG_FILE, "a");
	if (!logfile) {
		exit(EXIT_FAILURE);
	}
	fprintf(logfile, "cecd started.\n");
	fflush(logfile);

	libcec_set_logging(LIBCEC_LOG_LEVEL_DEBUG, logfile);
	libcec_init();
	if (libcec_open(CEC_DEVICE, &handle) <0) {
		fprintf(logfile, "cannot open CEC device\n");
		goto out;
	}

	// Per the CEC spec, logical addresses 4, 8, and 11 are reserved for playback devices
	if (libcec_set_logical_address(handle, 4) < 0) {
		fprintf(stderr, "failed to set CEC logical address\n");
		goto out1;
	}

	while(1) {
		len = libcec_receive_message(handle, buffer, 128);
		if (len <= 0) {
			fprintf(logfile, "Could not read message (error %d)\n", len);
			goto out1;
		}
		libcec_decode_message(buffer, len);
#define XTREAMER_TEST
#ifdef XTREAMER_TEST
		buffer[0] = 0x40;	// 4->0
		switch(buffer[1]) {
		case 0x46:	// Give OSD Name
			buffer[1] = 0x47;	// Set OSD Name
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
		case 0x8C:	// Give Device Vendor ID
			buffer[1] = 0x87;	// Report Vendor ID
			buffer[2] = 0x00;
			buffer[3] = 0x1C;
			buffer[4] = 0x85;	// Eunicorn Korea
			len = 5;
			break;
		case 0x8F:	// Give Device Power Status
			buffer[1] = 0x90;	// Power Status
			buffer[2] = 0x00;	// ON
			len = 3;
			break;
		case 0x9F:	// Get CEC Version
			buffer[1] = 0x9E;	// CEC Version
			buffer[2] = 0x04;	// version 1.3a
			len = 3;
			break;
		case 0x87:	// Device Vendor ID is a good time to send a request
			buffer[1] = 64;	// Set OSD String
			buffer[2] = 0;
			buffer[3] = 'H';
			buffer[4] = 'e';
			buffer[5] = 'l';
			buffer[6] = 'l';
			buffer[7] = 'o';
			buffer[8] = '!';
			len = 9;
			break;
		default:
			len = 0;
			break;
		}
		if (len) {
			if (libcec_send_message(handle, buffer, len)) {
				fprintf(logfile, "Could not send message\n");
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
	unlink(LOCK_FILE);
	fprintf(logfile, "cecd stopped.\n");
	fclose(logfile);
	exit(ret_val);
}
