#!/usr/bin/python
# File Name: perf_count.p

import sys

output = sys.argv[1]

data = open(output, 'r')
activities = data.readlines()
data.close()

val = []

for value_str in activities:
    val.append((float)(value_str.split(" ")[0]))

val.sort()

for i in range(len(val)):
    print '{0} {1}'.format(val[i]/1000, (i+1)*1.0/len(val))
