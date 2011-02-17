#!/usr/bin/python

import sys

fields = [
	"Major","Minor", 'dev', 'reads_issued', 'reads_merged', 'sectors_read',
	'ms_reads', 'writes_completed', 'writes_merged', 'sectors_write', 'ms_writes',
	'ios_in_prog', 'weighted']

def line2diskstat(line):
	global fields
#Field 1 -- # of reads issued
#Field 2 -- # of reads merged, field 6 -- # of writes merged
#3Field 3 -- # of sectors read
#Field 4 -- # of milliseconds spent reading
#Field 5 -- # of writes completed
#Field 7 -- # of sectors written
#Field 8 -- # of milliseconds spent writing
#Field 9 -- # of I/Os currently in progress
#Field 10 -- # of milliseconds spent doing I/Os
#Field 11 -- weighted # of milliseconds spent doing I/Os
	s = line.split()
	diskstat = dict()
	i = 0
	for f in fields:
		v = s[i]
		if i > 2: v = int(v)
		diskstat[f] = v
		i = i + 1
	return diskstat

def readStatMap(f):
	statmap = dict()
	for line in f:
		diskstat = line2diskstat(line)
		statmap[diskstat['dev']] = diskstat
	return statmap

def dsDiff(ds_begin, ds_end):
	ds_delta = dict()
	for k in ds_end.keys():
		if isinstance(ds_end[k], str):
			ds_delta[k] = ds_end[k]
		else:
			ds_delta[k] = ds_end[k] - ds_begin[k]
	return ds_delta

begin_map = readStatMap(open(sys.argv[1]))
end_map = readStatMap(open(sys.argv[2]))


for f in fields:
	sys.stdout.write(f + " / ")
print ''

for k in begin_map.keys():
	diskstat = dsDiff(begin_map[k], end_map[k])
	for f in fields:
		sys.stdout.write(str(diskstat[f]) + " ")
	print ''