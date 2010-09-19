#!/bin/bash

source `pwd`/tests/test_common.sh

tool_name="browser-vfat"
img_name="vfat.img"

# visit virtual file (inode 0)
run_test 1 "Virtual Visit"
run_test 2 "Virtual Dump"

exit 0
