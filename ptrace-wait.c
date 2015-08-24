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
 * ptrace-wait - Wait for any process to finish, or for it to run a given
 * number of instructions.
 *
 * Written by Aaron Lindsay and Christopher Covington.
 *
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <asm/unistd.h>

#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

#include <linux/perf_event.h>
#include <linux/ptrace.h>

static int setup_count(long long count, pid_t pid)
{
	struct perf_event_attr pe;
	int pfd = -1;

	memset(&pe, 0, sizeof(struct perf_event_attr));
	pe.type = PERF_TYPE_HARDWARE;
	pe.size = sizeof(struct perf_event_attr);
	pe.config = PERF_COUNT_HW_INSTRUCTIONS;
	pe.sample_period = count;
	pe.inherit = 1;
	pe.pinned = 1;
	pe.exclude_kernel = 1;
	pe.exclude_hv = 1;
	pe.watermark = 1;
	pe.wakeup_watermark = 1;
	printf("Waiting %lld instructions for PID %d\n", count, pid);
	pfd = syscall(__NR_perf_event_open, &pe, pid, -1, -1, 0);
	if (pfd < 0) {
		perror(NULL);
		printf("Error setting up instruction counting\n");
		exit(EXIT_FAILURE);
	}
	fcntl(pfd, F_SETOWN, pid);
	fcntl(pfd, F_SETFL, fcntl(pfd, F_GETFL) | FASYNC);
	ioctl(pfd, PERF_EVENT_IOC_RESET, 0);
	return pfd;
}

int main(int argc, char *argv[])
{
	pid_t pid, waitedpid;
	int fd, status, ret = 0;
	char *end;
	long long count;

	if (argc != 2 && argc != 3) {
		fprintf(stderr, "Usage: %s pid [instruction-count]\n"
			"\n"
			"Wait for any process to finish or run a given number of instructions.\n",
			argv[0]);
		return -1;
	}

	errno = 0;
	pid = strtol(argv[1], &end, 10);
	if (errno) {
		perror("Invalid PID");
		return -1;
	} else if (end == argv[1] || *end != '\0') {
		fprintf(stderr, "Invalid PID\n");
		return -1;
	}

	if (ptrace(PTRACE_SEIZE, pid, 0, PTRACE_O_TRACEEXIT)) {
		fprintf(stderr, "PTRACE_SEIZE returned error, %d\n", errno);
		return -1;
	}

	/* TODO: Use cgroup if there is more than one thread involved */
	if (argc == 3) {
		errno = 0;
		count = strtoll(argv[2], &end, 0);
		if (errno) {
			perror("Invalid instruction count");
			return -1;
		} else if (end == argv[2] || *end != '\0') {
			fprintf(stderr, "Invalid instruction count\n");
			return -1;
		}
		fd = setup_count(count, pid);
		kill(pid, SIGCONT);
	}

	while (1) {
		waitedpid = waitpid(pid, &status, 0);
		if (waitedpid != pid) {
			fprintf(stderr, "Error waiting for pid %d: %d\n", pid,
				waitedpid);
			ret = -1;
			break;
		}

		/* Exit if the process we're waiting on exited */
		if ((status>>8) == (SIGTRAP | (PTRACE_EVENT_EXIT<<8)))
			break;

		if (WIFSTOPPED(status)) {
			if ((status>>16 != PTRACE_EVENT_STOP) ||
					(WSTOPSIG(status) != SIGSTOP &&
					 WSTOPSIG(status) != SIGTSTP &&
					 WSTOPSIG(status) != SIGTTIN &&
					 WSTOPSIG(status) != SIGTTOU)) {
				if (argc == 3 && WSTOPSIG(status) == SIGIO) {
					/*
					 * If we set up an instruction count,
					 * assume that any SIGIO is caused by
					 * the perf event and transform it into
					 * a SIGSTOP.  Once CRIU has support
					 * for cgroup frozen processes, using
					 * that may be preferable to SIGSTOP.
					 */
					ptrace(PTRACE_CONT, pid, 0, SIGSTOP);
					break;
				}

				/*
				 * Handle signal-delivery-stop by
				 * passing along the signal to the
				 * tracee via PTRACE_CONT, as long as
				 * it wasn't a group-stop for a
				 * stopping signal.
				 */
				ptrace(PTRACE_CONT, pid, 0, WSTOPSIG(status));
			} else {
				/*
				 * If we received a group-stop (like SIGSTOP),
				 * PTRACE_LISTEN instead of
				 * PTRACE_CONT. This allows the tracee to
				 * remain in the stopped state (as if it
				 * received SIGSTOP while not being traced),
				 * while allowing it to receive future SIGCONT
				 * signals. If we use PTRACE_CONT in the
				 * SIGSTOP case, the tracee will run instead of
				 * remaining in the stopped state as we expect.
				 * See the 'Group-stop' section in `man 2
				 * ptrace` for more information about this
				 * behavior.
				 */
				ptrace(PTRACE_LISTEN, pid, 0, 0);
			}
		} else {
			fprintf(stderr,
				"Unexpected wakeup from waitpid for pid %d\n",
				pid);
			ret = -1;
			break;
		}
	}

	if (argc == 3) {
		fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & !FASYNC);
		if (read(fd, &count, sizeof(count)) != sizeof(count))
			fprintf(stderr, "Could not read from perf event\n");
		else
			printf("Counted %lld instructions\n", count);
		close(fd);
	}

	printf("Detaching from pid %d\n", pid);
	ptrace(PTRACE_DETACH, pid);

	return ret;
}
