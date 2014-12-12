#!/bin/bash
for i in `seq 0 2047`;
do
	./search_cache $i
done
