#!/bin/bash
if [ "$1" == original ]; then
	rm -rf original_assoc_num
	for i in `seq 0 2047`;
	do
		for j in `seq 0 5`;
		do
			./check_conflicts_set $i $j 0
		done
	done
fi

if [ "$1" == interfere ]; then
	rm -rf interfere_assoc_num
	for i in `seq 0 2047`;
	do
		for j in `seq 0 5`;
		do
			./check_conflicts_set $i $j 1
		done
	done
fi
