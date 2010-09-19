#!/bin/bash

tool_name="browser-vfat"
img_name="vfat.img"
source `pwd`/tests/test_common.sh

run_test 1 "Visit Root DE[1]"
run_test_no_match 2 "Virt-If. Non-File" "vfile"
run_test_match 3 "Virt-If. File." "vfile"

# don't get boot sector!!
run_test_no_match 4 "Virtual. Read file xlate." "55 aa"

exit 0
