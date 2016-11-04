#!/usr/bin/python

import os
import sys
import string

from collections import OrderedDict

data = open("res", 'r')
activities = data.readlines()
data.close()

num = {}

for activity in activities:
    activity_list = activity.split(" ")
    _sum = 0
    key = activity_list[0]
    for val in activity_list[1:-1]:
        if not val.isdigit():
            print val
        _sum += int(val)

    if key in num:
        num[key] += _sum
    else:
        num[key] = _sum

num_sorted = OrderedDict(sorted(num.items(), key=lambda x: x[1]))

for k, v in num_sorted.items():
    print "{0}: {1}".format(k, v)
