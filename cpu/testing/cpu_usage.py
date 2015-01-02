#!/usr/bin/python
# File Name: cpu_usage.py

"""
	This file is used to process the data from the xentrace output
"""

import os
import string
import sys 
import getopt

CPU_MASK = "0x00000001"
EVENT_MASK = "0x2f000"
TIME = "10"

##
# Command:
# $ xentrace -D -c 0 -e 0x2f000 -T 10 result
# $ cat result | xentrace_format formats | grep switch_infprev > output
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

	print "-------------------------CPU Activity:---------------------------"
	for activity in activities:
		activity_list = activity.split(' ')
		for i in range(len(activity_list)):
			if (activity_list[i] == "old_domid"):
				domid = int((activity_list[i+2])[:-1], 0)
				for j in range(len(tid_list)):
					if (domid == (int)(tid_list[j], 0)):
						period = (int)(activity_list[i+5])
						print '{0}: {1}'.format(domid, period)
						runtime_list[j] += period

	print "------------------------Relative Usage---------------------------"
	total_time = 0
	for i in range(len(tid_list)):
		total_time = total_time + runtime_list[i]

	for i in range(len(tid_list)):
		ratio_usage = runtime_list[i] * 1.0 / total_time
		print '{0}: {1}'.format(tid_list[i], ratio_usage)


def main(argv):
	tid_list = argv
	if (len(tid_list) == 0):
		print 'Input VM id'
		sys.exit()

	execute_cmd(CPU_MASK, EVENT_MASK, TIME)

	process(tid_list)


if __name__ == "__main__":
	main(sys.argv[1:])

