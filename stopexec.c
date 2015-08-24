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
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	FILE *fp;

	if (argc < 3) {
		printf("Typical usage: %s logfile -- program [args...] &\n",
		       argv[0]);
		return -1;
	}

	fp = fopen(argv[1], "w");
	if (fp == NULL) {
		printf("Error opening %s.\n", argv[1]);
		return errno;
	}

	/* Set session ID to make process easily `criu dump`-able */
	setsid();
	fprintf(fp, "Stopping pid %d before exec().\n", getpid());
	fclose(fp);
	raise(SIGSTOP);

	return execvp(argv[3], &argv[3]);
}
