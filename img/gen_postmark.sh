#!/bin/bash

sudo chmod -R 777 /mnt/FSTEST
rm -rf /mnt/FSTEST/*

if [ -z "$GEN_FILE_PATH" ]; then
	GEN_FILE_PATH=/mnt/FSTEST
fi

if [ -z "$MAX_FILE_BYTES" ]; then
	MAX_FILE_SIZE=20000
fi

if [ -z "$MIN_FILE_BYTES" ]; then
	MIN_FILE_BYTES=100
fi

if [ -z "$MAX_FILES" ]; then
	MAX_FILES=5000
fi

if [ -z "$NUM_SUBDIRS" ]; then
	NUM_SUBDIRS=5
fi

../benchmarks/postmark/postmark <<<"
set size ${MIN_FILE_BYTES} ${MAX_FILE_BYTES}
set number ${MAX_FILES}
set location ${GEN_FILE_PATH}
set subdirectories ${NUM_SUBDIRS}
run"
