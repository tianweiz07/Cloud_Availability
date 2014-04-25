#!/usr/bin/python
# File Name: process.py

"""
	This file is used to process the data from the lttng output
"""

import os
import string

def process(tid_list):

	runtime_list = [0]*len(tid_list);

	data = open('result', 'r')
	activities = data.readlines()
	data.close()

	activity_start = activities[0]
	hour_start = int(activity_start.split(' ')[0][1:3])
	minute_start = int(activity_start.split(' ')[0][4:6])
	second_start = int(activity_start.split(' ')[0][7:9])
	millisecond_start = int(activity_start.split(' ')[0][10:-1])

	activity_end = activities[-1]
	hour_end = int(activity_end.split(' ')[0][1:3])
	minute_end = int(activity_end.split(' ')[0][4:6])
	second_end = int(activity_end.split(' ')[0][7:9])
	millisecond_end = int(activity_end.split(' ')[0][10:-1])

	total_time = ((hour_end-hour_start)*3600 + (minute_end-minute_start)*60 + (second_end-second_start))*1000000000 + (millisecond_end-millisecond_start)

	for activity in activities:
		prev_tid = activity.split(' ')[14]
		next_tid = activity.split(' ')[26]
		for i in range(len(tid_list)):
			if (prev_tid == tid_list[i]):
				hour = int(activity.split(' ')[0][1:3])
				minute = int(activity.split(' ')[0][4:6])
				second = int(activity.split(' ')[0][7:9])
				millisecond = int(activity.split(' ')[0][10:-1])
				runtime_list[i] += (hour*3600 + minute*60 + second)*1000000000 + millisecond
			if (next_tid == tid_list[i]):
				hour = int(activity.split(' ')[0][1:3])
				minute = int(activity.split(' ')[0][4:6])
				second = int(activity.split(' ')[0][7:9])
				millisecond = int(activity.split(' ')[0][10:-1])
				runtime_list[i] -= (hour*3600 + minute*60 + second)*1000000000 + millisecond
#	print runtime
#	print total_time
	for i in range(len(tid_list)):
		print runtime_list[i]*1.0/total_time

process(['4119,','4703,','5381,','6044,','6698,','7442,','8187,','9829,','11051,','11902,'])
