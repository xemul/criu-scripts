/*
 * Copyright (c) 2015, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 and only version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * stopexec - Stop an application immediately prior to execution.
 *
 * Written by Christopher Covington.
 */

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	int fd, err;

	if (argc < 3) {
		printf("Typical usage: %s logfile -- program [args...] &\n",
		       argv[0]);
		return -1;
	}

	fd = open(argv[1], O_WRONLY | O_CREAT | O_CLOEXEC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
	if (fd < 0) {
		printf("Error opening %s.\n", argv[1]);
		return errno;
	}

	/* Set session ID to make process easily `criu dump`-able */
	setsid();
	dprintf(fd, "Stopping pid %d before exec().\n", getpid());
	raise(SIGSTOP);

	/* Run application */
	execvp(argv[3], &argv[3]);
	err = errno;

	/* Log and return error if execvp failed */
	dprintf(fd, "Error executing binary: %s (errno: %d)\n", strerror(err), err);
	close(fd);
	return err;
}
