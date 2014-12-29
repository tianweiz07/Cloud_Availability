#!/bin/bash
rm -rf conflict_sets
for i in `seq 0 2047`;
do
	./search_cache $i
done
