#!/usr/bin/python
# File Name: diff.py

"""
	This file is used to process the data from two different files
"""

import os
import string
import sys

def process(file1, file2):
	data1 = open(file1, 'r')
	activities1 = data1.readlines()
	data1.close()

	data2 = open(file2, 'r')
	activities2 = data2.readlines()
	data2.close()

	for i in range(2048):
		first0 = int(activities1[i].split(' ')[0])
		second0 = int(activities2[i].split(' ')[0])
		diff0 = second0 - first0
		first1 = int(activities1[i].split(' ')[1])
		second1 = int(activities2[i].split(' ')[1])
		diff1 = second1 - first1
		first2 = int(activities1[i].split(' ')[2])
		second2 = int(activities2[i].split(' ')[2])
		diff2 = second2 - first2
		first3 = int(activities1[i].split(' ')[3])
		second3 = int(activities2[i].split(' ')[3])
		diff3 = second3 - first3
		first4 = int(activities1[i].split(' ')[4])
		second4 = int(activities2[i].split(' ')[4])
		diff4 = second4 - first4
		first5 = int(activities1[i].split(' ')[5])
		second5 = int(activities2[i].split(' ')[5])
		diff5 = second5 - first5
#		if (diff0 < 1000):
#			diff0 = 0
#		if (diff1 < 1000):
#			diff1 = 0
#		if (diff2 < 1000):
#			diff2 = 0
#		if (diff3 < 1000):
#			diff3 = 0
#		if (diff4 < 1000):
#			diff4 = 0
#		if (diff5 < 1000):
#			diff5 = 0
		print diff0, diff1, diff2, diff3, diff4, diff5
	
process(sys.argv[1], sys.argv[2])
