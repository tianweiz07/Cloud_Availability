#!/usr/bin/python
# File Name: process.py

"""
	This file is used to process the data from the xentrace output
"""

##
# Command:
# $ xentrace xentrace -D -e 0x2f000 -T 100 binary_result
# $ cat binary_result | xentrace_format format > result
#

import os
import string

Frequency = 3.292606

def process(tid_list):

	runtime_list = [0]*len(tid_list);

	data = open('output', 'r')
	activities = data.readlines()
	data.close()

	activity_start = activities[0]
	start_time = int(activity_start.split(' ')[2])

	activity_end = activities[-1]
	end_time = int(activity_end.split(' ')[2])

	total_time = end_time - start_time

	for activity in activities:
		activity_list = activity.split(' ')
		for i in range(len(activity_list)):
			if (activity_list[i] == "old_domid"):
				domid = activity_list[i+2]
				for j in range(len(tid_list)):
					if (domid == tid_list[j]):
						runtime_list[j] += (int)(activity_list[i+5])

		
	print total_time
	for i in range(len(tid_list)):
		print runtime_list[i]*Frequency/total_time

process(['0x0000002e,','0x00000015,'])
