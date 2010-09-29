#!/usr/bin/python
import os
import sys

from RunStats import *

file_systems = [ "vfat", "ext2", "nilfs" ]
tests_by_fs = dict()
for fs in file_systems:
	tests_by_fs[fs] = []

def process_dir(d):
	stat_time=d.split('.')[0]
	files=os.listdir(d)
	test_names=(open(d+'/tests.log','r')).readlines()
	i = 0
	files.sort()
	for f in files:
		if f[0] == 't':
			# test.log
			continue
		for fs in file_systems:
			if fs in test_names[i]:
				rs = RunStats(stat_time,d+"/"+f,test_names[i])
				tests_by_fs[fs].append(rs)
				break
		i = i+1


def process_fs_tests(fs_name, run_stat_list):
	sorted(run_stat_list, key = lambda rs : rs.stat_time)
	print fs_name
	for rs in run_stat_list:
		print rs.test_name
		for k in rs.params.keys():
			print k +": " + rs.get(k)

args=sys.argv
args.pop(0)
for d in args:
	process_dir(d)


for fs in file_systems:
	process_fs_tests(fs, tests_by_fs[fs])
