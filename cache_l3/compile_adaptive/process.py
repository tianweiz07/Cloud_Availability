#/usr/bin/python

import sys
import string
import os

from collections import OrderedDict

directory = sys.argv[1]

res = {}


for filename in os.listdir(directory):
    if filename.endswith(".c") or filename.endswith(".cpp"):
        os.popen("gcov " + filename)

for filename in os.listdir(directory):
    if filename.endswith(".gcov"): 
        data = open(filename, 'r')
        activities = data.readlines()
        data.close()

        for activity in activities:
            val1 = activity.split(":")[0]
            val1 = val1.replace(" ", "")
            val2 = activity.split(":")[1]
            val2 = val2.replace(" ", "")

            if val1.isdigit():
                res[filename + ":"+val2] = int(val1)

res_sorted = OrderedDict(sorted(res.items(), key=lambda x: x[1]))

for k, v in res_sorted.items():
    print "{0}: {1}".format(k, v)
