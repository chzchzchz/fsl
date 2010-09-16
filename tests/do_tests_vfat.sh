#!/bin/bash

src_root=`pwd`

# visit disk.root_dir[1]
cmd="${src_root}/src/tool/browser-vfat ${src_root}/img/vfat.img 2>&1 < ${src_root}/tests/vfat-test-input-1"
echo "$cmd" >failed_test_cmd
echo "$cmd" >>tests.log
outstr=`eval $cmd`
retval=$?

if [ $retval -ne 0 ]; then
	echo "$outstr"
fi

exit $retval
