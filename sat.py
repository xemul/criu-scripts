#!/bin/env python
#
# Strace-Alanyzing-Tool -- a script that helps finding out
# where CRIU spends time in the kernel while working
#
# For dump:
# # strace -o log -T criu dump ... -R
#
# For restore
# # strace -o log -f -T criu restore ... --action-script $(pwd)/scripts/fake-restore.sh
#
# Then
# # cat log | sat.py [-t|-c]
#   -t -- sort by total time spent
#   -c -- sort by the amount of calls performed
#

import sys

raw = False

NONE = 0
COUNT = 1
TIME = 2

sort = NONE

if len(sys.argv) >= 2:
	if sys.argv[1] == "-r":
		raw = True
	elif sys.argv[1] == "-c":
		sort = COUNT
	elif sys.argv[1] == "-t":
		sort = TIME

data = sys.stdin.readlines()
stats = {}
tot = (0, 0.0)

for ln in data:
	ln = ln.strip()
	if not ln[0].isdigit():
		# No per-pid stuff
		ln = "1  " + ln

	try:
		sysc_l = ln.split(" ", 1)[1]
		if sysc_l.startswith(" <..."):
			# Resume: " <... open resumed"
			sysc = sysc_l.split(" ", 3)[2]
		else:
			# Call: "open(..."
			sysc = sysc_l.split("(", 1)[0]
			if sysc.startswith(" ---"):
				# Signals
				continue
			if sysc.startswith(" +++"):
				# Exits
				continue
			if sysc.startswith(" exit"):
				# Doesn't matter
				continue
	except IndexError:
		print "No syscall name in [%s]" % ln
		raise(Exception())

	try:
		time_s = ln.rsplit("<", 1)[1].strip(">")
		if time_s.startswith("unfinished"):
			# Concurrent processes
			continue
	except IndexError:
		print "No times in [%s] -> [%s]" % (ln, sysc)
		raise(Exception())

	try:
		time = float(time_s)
	except ValueError:
		print "Time is bad in [%s] -> [%s] [%s]" % (ln, sysc, time_s)
		raise(Exception())

	sysc = sysc.strip()
	if not stats.has_key(sysc):
		stats[sysc] = (1, time)
	else:
		stats[sysc] = (stats[sysc][0] + 1, stats[sysc][1] + time)

	tot = (tot[0] + 1, tot[1] + time)

stats_r = map(lambda i: (i, stats[i][0], stats[i][1]), stats)
if sort != NONE:
	stats_r.sort(key=lambda t: t[sort])

if not raw:
	stats_r.append(("Total", tot[0], tot[1]))

tot_prc = 0
half = False
quat = False
for k in stats_r:
	prc = k[2] * 100. / tot[1]
	suf = ""

	if sort == TIME:
		tot_prc += prc
		if not half and tot_prc >= 50:
			half = True
			suf = " <- 50%"
		if not quat and tot_prc >= 25:
			quat = True
			suf = " <- 75%"

	print "%20s: %8d  %f (%3.1f%%)%8s" % (k[0], k[1], k[2], prc, suf)
