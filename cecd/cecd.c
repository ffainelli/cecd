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

#define RUNNING_DIR	"/tmp"
#define LOCK_FILE	"/var/lock/cecd.lock"
#define LOG_FILE	"/var/log/cecd.log"
#define CEC_DEVICE	"/dev/cec/0"

enum {
	CEC_ENABLE,
	CEC_SET_LOGICAL_ADDRESS,
	CEC_SET_POWER_STATUS,
	CEC_SEND_MESSAGE,
	CEC_RCV_MESSAGE,
};

typedef struct {
	unsigned char*		buf;
	unsigned char		len;
} cec_msg;


static FILE* logfile = NULL;
static int lock_fd;

void display_buffer_hex(unsigned char *buffer, unsigned size)
{
	unsigned i;

	for (i=0; i<size; i++) {
		if (!(i%0x10))
			fprintf(logfile, "  ");
		fprintf(logfile, " %02X", buffer[i]);
	}
	fprintf(logfile, "\n");
	fflush(logfile);
}

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
	int cec_dev, len, ret_val = 0;
	unsigned char buffer[128];
	cec_msg msg = {buffer, 128};

	daemonize();

	logfile = fopen(LOG_FILE, "a");
	if (!logfile) {
		exit(EXIT_FAILURE);
	}
	fprintf(logfile, "cecd started.\n");
	fflush(logfile);

	cec_dev = open(CEC_DEVICE, 0);
	if (cec_dev < 0) {
		fprintf(logfile, "cannot open CEC device %s\n", CEC_DEVICE);
		goto out;
	}

	ret_val = ioctl(cec_dev, CEC_ENABLE, 1);
	if (ret_val) {
		fprintf(logfile, "cannot enable CEC device\n");
		goto out1;
	}

	// Per the CEC spec, logical addresses 4, 8, and 11 are reserved for playback devices
	ret_val = ioctl(cec_dev, CEC_SET_LOGICAL_ADDRESS, 4);
	if (ret_val) {
		fprintf(stderr, "failed to set CEC logical address\n");
		goto out1;
	}

	while(1) {
		len = ioctl(cec_dev, CEC_RCV_MESSAGE, &msg);
		if (len <= 0) {
			fprintf(logfile, "Could not read message (error %d)\n", len);
			goto out1;
		}
		display_buffer_hex(buffer, (unsigned)len);
	}

out1:
	close(cec_dev);
out:
	close(lock_fd);
	unlink(LOCK_FILE);
	fprintf(logfile, "cecd stopped.\n");
	fclose(logfile);
	exit(ret_val);
}
