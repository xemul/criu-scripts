/*
 * Copyright (c) 2015, 2016 The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 and only version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 */

#define _GNU_SOURCE // dprintf
#include <stdlib.h> // EXIT_*
#include <stdio.h> // printf, fprintf
#include <string.h> // strerror
#include <unistd.h> // fork, execvp, setsid
#include <errno.h> // errno
#include <fcntl.h> // open
#include <signal.h> // kill
#include <assert.h> // assert
#include <sys/stat.h> // open
#include <sys/ptrace.h> // ptrace
#include <sys/types.h> // waitpid
#include <sys/wait.h> // waitpid

int main(int argc, char *argv[])
{
	int fd;

	if (argc < 3) {
		printf("Typical usage: %s logfile -- program [args...] &\n",
			argv[0]);
		return EXIT_FAILURE;
	}

	fd = open(argv[1], O_TRUNC | O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
	if (fd < 0) {
		fprintf(stderr, "Error opening %s.\n", argv[1]);
		return errno;
	}

	pid_t pid = fork();
	if (pid) {
		int status;
		waitpid(pid, &status, 0);
		/* Wait until child has stopped to write its PID so it can act
		   as barrier of sorts. */
		dprintf(fd, "%d", pid);
		close(fd);
		if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP) {
			ptrace(PTRACE_DETACH, pid, 0, SIGSTOP);
			return EXIT_SUCCESS;
		} else {
			fprintf(stderr, "Child stopped unexpectedly\n");
			kill(pid, SIGKILL);
			return EXIT_FAILURE;
		}
	} else {
		close(fd);
		/* Set session ID to make process easily `criu dump`-able */
		setsid();
		int status = ptrace(PTRACE_TRACEME, 0, 0, 0);
		if (status == -1) {
			fprintf(stderr, "Trace request failed with error: %s\n",
				strerror(errno));
			return EXIT_FAILURE;
		}
		status = execvp(argv[3], &argv[3]);
		if (status == -1) {
			fprintf(stderr, "Exec of %s failed with error: %s\n",
				argv[3], strerror(errno));
		}
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
