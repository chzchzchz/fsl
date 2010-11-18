#!/bin/bash

if [ -z $1 ]; then
	BRANCH_FACTOR=5
else
	BRANCH_FACTOR=$1
fi

if [ -z "$GEN_FILE_PATH" ]; then
	GEN_FILE_PATH=/mnt/FSTEST
fi

sudo chmod -R 777 "${GEN_FILE_PATH}"

for a in `seq ${BRANCH_FACTOR}`; do
	echo "${a}.aaa " >"${GEN_FILE_PATH}"/"$a".aaa
done

for b in `seq ${BRANCH_FACTOR}`; do
	mkdir -p "${GEN_FILE_PATH}"/"$b"d
	for a in `seq ${BRANCH_FACTOR}`; do
		echo "${b}d/$a.qqq " >"${GEN_FILE_PATH}"/"$b"d/$a.qqq
	done
done
