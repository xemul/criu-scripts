#!/usr/bin/env python
"""Process tree continuer

pstree_cont - a script that sends SIGCONT signal to each process in a process tree.
PID of process tree root is passed as the first script argument.

Usage:
$ ./pstree_cont.py <root_pid>
"""

import sys
import os
import signal

opt_pstree_root_pid = 0


def parse_args(args):
    global opt_pstree_root_pid
    if (len(args) > 1) and args[1].isdigit():
        opt_pstree_root_pid = int(args[1])
        return True
    else:
        return False


def cont_children(pid):
	f_children_path = "/proc/{0}/task/{0}/children".format(pid)
	f_children = open(f_children_path, "r")

	try:
		for line in f_children:
			for child_pid in line.strip().split(" "):
				if not child_pid.isdigit():
					err = ("Error while getting children of {}.\n"
						"Error occured while parsing '{}'.\n"
						"Line '{}' could not be parsed correctly.\n"
						"'{}' is not a valid process id (should be integer).\n")
					err = err.format(pid, f_children.name, line.strip(), child_pid)
					raise ValueError(err)
				cont_children(int(child_pid))

		os.kill(pid, signal.SIGCONT)
	finally:
		f_children.close()


def pr_usage(program_name):
	sys.stdout.write("Usage:\n{} <root_pid>\n".format(program_name))


def main():
	if not parse_args(sys.argv):
		pr_usage(sys.argv[0])
		sys.exit(1)

	cont_children(opt_pstree_root_pid)


if __name__ == "__main__":
	main()
