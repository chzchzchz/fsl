#!/bin/bash

sudo chmod -R 777 /mnt/FSTEST
rm -rf /mnt/FSTEST/*

../benchmarks/postmark/postmark <<<"
set size 100 20000
set number 5000
set location /mnt/FSTEST/
set subdirectories 5
run"
