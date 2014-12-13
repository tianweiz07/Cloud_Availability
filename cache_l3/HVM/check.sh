#!/bin/bash
for i in `seq 0 2047`;
do
	for j in `seq 0 5`;
	do
		./check_conflicts $i $j 20
	done
	echo -e " "
done
