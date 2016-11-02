#!/bin/env python

import sys
import subprocess

merged = set()
subprocess.check_call(["git", "checkout", "master"])
l = subprocess.Popen(["git", "log", "%s.." % sys.argv[1], "--pretty=short"], stdout = subprocess.PIPE)
for c in l.stdout:
	if c.startswith(' '):
		merged.add(c.strip())
l.wait()

unmerged = []
cid = None
subprocess.check_call(["git", "checkout", "criu-dev"])
l = subprocess.Popen(["git", "log", "%s.." % sys.argv[1], "--pretty=short"], stdout = subprocess.PIPE)
for c in l.stdout:
	if c.startswith('commit'):
		if cid:
			raise Exception('git log output screwed')
		cid = c.split()[1].strip()[:7]
	elif c.startswith(' '):
		if not cid:
			raise Exception('git log output screwed')
		c = c.strip()
		if not c in merged:
			unmerged.append('pick %s %s' % (cid, c))
		cid = None
l.wait()

unmerged.reverse()
for c in unmerged:
	print c
