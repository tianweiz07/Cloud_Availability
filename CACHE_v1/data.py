import os
import string

def process_data():
	data_list = [0]*12288
	data_file = open('log', 'r')
	data_lines = data_file.readlines()
	data_file.close()
	index = 0

	for data_line in data_lines:
		data_list[index] = int(data_line.split(' ')[0])
		index = index + 1
		data_list[index] = int(data_line.split(' ')[1])
		index = index + 1
		data_list[index] = int(data_line.split(' ')[2])
		index = index + 1
		data_list[index] = int(data_line.split(' ')[3])
		index = index + 1
		data_list[index] = int(data_line.split(' ')[4])
		index = index + 1
		data_list[index] = int(data_line.split(' ')[5])
		index = index + 1
	
	data_list.sort()
	data_max = data_list[-1]
	count = 0
	for i in range(data_max):
		while ((data_list[count]<=i) and (count<12288)):
			count = count + 1
		print '{}'.format((count-1)*1.0/12288)

process_data()
