#!/usr/bin/python

import os

class RunStats:
	def __init__(self, stat_time, fname,test_name):
		lines=(open(fname,'r')).readlines()
		self.params=dict()
		for l in lines:
			s=l.split(' ')
			param_name=s[0]
			param_val=s[1]
			self.params[s[0]] = ((s[1]).rstrip())
		self.stat_time= stat_time
		self.test_name = test_name.rstrip()


	def get(self, param_name):
		return self.params[param_name]
