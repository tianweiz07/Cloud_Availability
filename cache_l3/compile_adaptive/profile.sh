#!/bin/bash
rm -rf profiling_res
for j in `seq 0 10`;
do
	for i in `seq 0 31`;
	do
		./check_conflicts_timing $(expr $i \* 64 + $1)
	done
done

python cal.py | tail -n 5
