#!/bin/bash

tool_name="browser-ext2"
img_name="ext2.img"
source `pwd`/tests/test_common.sh

# visit virtual file (inode 0)
run_test 1 "Virtual Visit"
run_test 2 "Virtual Dump"

exit 0
