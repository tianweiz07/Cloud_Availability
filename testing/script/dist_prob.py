#!/usr/bin/python
# File Name: perf_count.p

import sys

output = sys.argv[1]

data = open(output, 'r')
activities = data.readlines()
data.close()

_range = 10000
_bin = 20
_num = 0
_max = 0

val = [0]*_bin

for value_str in activities:
    value = (float)(value_str.split(" ")[0])
    index = (int)(value*_bin/_range)
    if index < _bin:
        val[index] += 1
        _num += 1
    else:
        val[-1] += 1
        _num += 1
    if _max < value:
        _max = value

print _max

for i in range(_bin):
    print '{0} {1}'.format(i, val[i])
