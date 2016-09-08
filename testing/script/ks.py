#!/usr/bin/python
# File Name: perf_count.p

import sys

output1 = sys.argv[1]
output2 = sys.argv[2]

data1 = open(output1, 'r')
activities1 = data1.readlines()
data1.close()

data2 = open(output2, 'r')
activities2 = data2.readlines()
data2.close()

val1 = []
val2 = []

for value_str in activities1:
    val1.append((float)(value_str.split(" ")[0]))

for value_str in activities2:
    val2.append((float)(value_str.split(" ")[0]))

val = val1 + val2

val1.sort()
val2.sort()
val.sort()

idx1 = idx2 = 0

ks = 0

for x in val:
    while idx1 < len(val1) and val1[idx1] <= x:
        idx1 += 1
    while idx2 < len(val2) and val2[idx2] <= x:
        idx2 += 1

    ks = max(ks, (idx1-1)*1.0/len(val1)-(idx2-1)*1.0/len(val2), (idx2-1)*1.0/len(val2)-(idx1-1)*1.0/len(val1))

print ks
