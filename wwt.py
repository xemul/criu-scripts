#!/usr/bin/env python

import sys

do_comp_stat = False
sys.argv.append("")
if sys.argv[1] == "--company":
	do_comp_stat = True

ppl_stat = {}
comp_stat = {}
del_weight = 0

def diffstat(lns):
	# Insertions - Deletions
	if lns[4].startswith("ins"):
		lines = int(lns[3])
		if len(lns) >= 6:
			lines -= int(lns[5]) * del_weight
	else:
		lines = -int(lns[3]) * del_weight

	return lines


author = None
total = (0, 0)
for ln in sys.stdin:
	ln = ln.strip()
	if len(ln) == 0:
		continue

	lns = ln.split(None, 1)
	if lns[0] == "Author:":
		author = lns[1]
		if do_comp_stat:
			try:
				author = author.rsplit("@", 1)[1].strip(">")
			except:
				author = "unknown" # authors w/o e-mail

		continue

	lns = ln.split()
	if len(lns) >= 3 and lns[1].startswith("file") and lns[2] == "changed,":
		if not author:
			print "Misordered output, no author"
			sys.exit(1)

		lines = diffstat(lns)
		commits = 1

		total = (total[0] + lines, total[1] + commits)

		if ppl_stat.has_key(author):
			lines += ppl_stat[author][0]
			commits += ppl_stat[author][1]

		ppl_stat[author] = (lines, commits)

		author = None
		continue

for k in ppl_stat:
	stat = ppl_stat[k]
	print "%10d (%3.0f%%) %10d (%3.0f%%)   %s" % (
			stat[0], stat[0] * 100. / total[0],
			stat[1], stat[1] * 100. / total[1],
			k)
