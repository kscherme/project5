#!/bin/bash

pol=$1
echo "# Frames, # Reads, # Writes, # Page Faults, Algorithm" >> ${pol}.csv
for alg in "sort" "scan" "focus"; do
    for i in 10 20 30 40 50 60 70 80 90 100; do
        out=$(./virtmem 100 $i $pol $alg)
	faults=$(echo "$out" | grep "page faults" | cut -d " " -f 5)
        reads=$(echo "$out" | grep "reads" | cut -d " " -f 5)
        writes=$(echo "$out" | grep "writes" | cut -d " " -f 5)
        echo "$i,$reads,$writes,$faults,$alg" >> ${pol}.csv
    done
done
