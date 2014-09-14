#!/usr/bin/python
# File Name: preemption_count.py

"""
	This file is used to process the data from the xentrace output
"""

import os
import string


CPU_MASK = "0x00000001"
EVENT_MASK = "0x2f000"
TIME = "10"
DOMAIN = "0x00000008,"

##
# Command:
# $ xentrace -D -c 0 -e 0x2f000 -T 10 result
# $ cat result | xentrace_formats format > output
#
def execute_cmd(cpu_mask, event_mask, time, domain):
	cmd0 = "rm -rf result output"
	cmd1 = "xentrace" + " -D" + " -c " + cpu_mask + " -e " + event_mask + " -T " + time + " result"
	cmd2 = "cat result | xentrace_format formats | grep switch_infprev " + "| grep " + domain + " > output"
	os.popen(cmd0)
	os.popen(cmd1)
	os.popen(cmd2)
	
	f = open('output', 'r')
	lines = f.readlines()
	print len(lines)
	f.close()

execute_cmd(CPU_MASK, EVENT_MASK, TIME, DOMAIN)
execute_cmd(CPU_MASK, EVENT_MASK, TIME, DOMAIN)
execute_cmd(CPU_MASK, EVENT_MASK, TIME, DOMAIN)
execute_cmd(CPU_MASK, EVENT_MASK, TIME, DOMAIN)
execute_cmd(CPU_MASK, EVENT_MASK, TIME, DOMAIN)
execute_cmd(CPU_MASK, EVENT_MASK, TIME, DOMAIN)
execute_cmd(CPU_MASK, EVENT_MASK, TIME, DOMAIN)
execute_cmd(CPU_MASK, EVENT_MASK, TIME, DOMAIN)
execute_cmd(CPU_MASK, EVENT_MASK, TIME, DOMAIN)
execute_cmd(CPU_MASK, EVENT_MASK, TIME, DOMAIN)
