#!/bin/bash
rm -rf conflict_sets
for i in `seq 0 31`;
do
	./search_cache $(expr $i \* 64 + $1)
done

rm -rf original_assoc_num
for i in `seq 0 31`;
do
	./check_conflicts_set $(expr $i \* 64 + $1)
done
