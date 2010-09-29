#!/bin/bash


sudo chmod -R 777 /mnt/FSTEST

for a in `seq 5`; do
	echo "A$a" >/mnt/FSTEST/"$a".aaa
done

for b in `seq 5`; do
	mkdir -p /mnt/FSTEST/"$b"d
	for a in `seq 5`; do
		echo "AAA$a" >/mnt/FSTEST/"$b"d/$a.qqq
	done
done
