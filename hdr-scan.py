#!/usr/bin/env python

import sys
import os

sdir = sys.argv[1]

def file_includes(path):
	incs = filter(lambda x: x.startswith('#include'), open(path))
	if not incs:
		return None

	incs = map(lambda x: x.strip().split()[1], incs)
	incs = filter(lambda x: x.startswith('"') and x.endswith('"'), incs)

	if not incs:
		return None

	return map(lambda x: x.strip('"'), incs)


def inc_dir(f):
	dirs = [ 'criu/include/', 'criu/arch/x86/include/' ]
	for d in dirs:
		if os.access(d + f, os.F_OK):
			return d

	return None


def files_in(d, ext):
	return filter(lambda x: x.endswith(ext), os.listdir(d))



def scan(p, seen_set, bad_set, dep = 0, c_path = []):
	print ' ' * dep + '%s' % p
	incs = file_includes(p)
	if not incs:
		# Terminating header
		print ' ' * dep + '---'
		return

	# Headers, that don't include other headers are
	# OK to be looped into. But those, that continue
	# the inclustion chain shouldn't meet any longer
	c_path.append(p)
	seen_set[p] = ' -> '.join(c_path)

	print ' ' * dep + '-->'
	for i in incs:
		d = inc_dir(i)
		if not d:
			if not i.startswith('images/'):
				print ' ' * dep + 'No dir for %s from %s' % (i, p)
			continue

		p = d + '/' + i
		if seen_set.has_key(p):
			print ' ' * dep + '! Already seen %s' % p
			if not bad_set.has_key(p):
				bad_set[p] = [ seen_set[p] ]
			bad_set[p].append(' -> '.join(c_path))

		scan(p, seen_set, bad_set, dep + 4, c_path)

	print ' ' * dep + '<--'
	c_path.pop()


bad_set = {}
for f in files_in(sdir, '.h'):
	seen_set = {}
	scan(sdir + '/' + f, seen_set, bad_set)

print 'Loopy headers:'
for i in bad_set:
	pl = bad_set[i]
	print '  %s (%d)' % (i, len(pl))
	for p in pl:
		print '           %s' % p
