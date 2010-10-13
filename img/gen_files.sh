#!/bin/bash

if [ -z $1 ]; then
	BRANCH_FACTOR=5
else
	BRANCH_FACTOR=$1
fi

sudo chmod -R 777 /mnt/FSTEST

for a in `seq ${BRANCH_FACTOR}`; do
	echo "A$a" >/mnt/FSTEST/"$a".aaa
done

for b in `seq ${BRANCH_FACTOR}`; do
	mkdir -p /mnt/FSTEST/"$b"d
	for a in `seq ${BRANCH_FACTOR}`; do
		echo "AAA$a" >/mnt/FSTEST/"$b"d/$a.qqq
	done
done
