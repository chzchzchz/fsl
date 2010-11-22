#!/bin/bash

tool_name="browser-testfs"
img_name="testfs.img"
source `pwd`/tests/test_common.sh

run_test_match 1 "Array indexes" "st4\[10\]@624--844"
run_test_match 2 "Follow array index" "v\[6\]@684--708"
run_test_match 3 "Parameterized type" "k\[3\]@848--860"
run_test_match 4 "Double parameterized type" "f2\[3\]@864--876"

exit 0
