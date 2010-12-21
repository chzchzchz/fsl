#!/bin/bash

if [ -z $1 ]; then
	DEPTH=50
else
	DEPTH=$1
fi

if [ -z "$FILES_IN_DIR" ]; then
	FILES_IN_DIR=5
fi

if [ -z "$GEN_FILE_PATH" ]; then
	GEN_FILE_PATH=/mnt/FSTEST
fi

sudo chmod -R 777 "${GEN_FILE_PATH}"

for b in `seq ${DEPTH}`; do
	GEN_FILE_PATH="${GEN_FILE_PATH}"/"$b"d
	mkdir -p "${GEN_FILE_PATH}"
	for a in `seq ${FILES_IN_DIR} `; do
		echo "${b}d/$a.qqq " >"${GEN_FILE_PATH}"/$a.qqq
	done
done
