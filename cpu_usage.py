#!/usr/bin/python
# File Name: process.py

"""
	This file is used to process the data from the xentrace output
"""

import os
import string


FREQUENCY = 3.292624
CPU_MASK = "0x000000ff"
EVENT_MASK = "0x2f000"
TIME = "10"
DOMAIN_ID1 = "0x00000039,"
DOMAIN_ID2 = "0x0000003a,"

##
# Command:
# $ xentrace -D -c 0 -e 0x2f000 -T 10 result
# $ cat result | xentrace_formats format > output
#
def execute_cmd(cpu_mask, event_mask, time):
	cmd0 = "rm -rf result output"
	cmd1 = "xentrace" + " -D" + " -c " + cpu_mask + " -e " + event_mask + " -T " + time + " result"
	cmd2 = "cat result | xentrace_format formats | grep switch_infprev > output"
	os.popen(cmd0)
	os.popen(cmd1)
	os.popen(cmd2)

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
		print runtime_list[i]*FREQUENCY/total_time


execute_cmd(CPU_MASK, EVENT_MASK, TIME)
process([DOMAIN_ID1,DOMAIN_ID2])
