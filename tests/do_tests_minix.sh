#!/bin/bash

tool_name="browser-minix"
img_name="minix.img"
source `pwd`/tests/test_common.sh

run_test_match 1 "Root directory" "Current: rootino\\[1\\]@12288"

exit 0
