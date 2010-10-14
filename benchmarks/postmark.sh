#!/bin/bash

# run postmark
benchmarks/postmark/postmark <<<"
set size 100 20000
set number 5000
set location /mnt/FSTEST/
set subdirectories 5
run"

exit -1