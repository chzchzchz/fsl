#!/bin/bash

tool_name="browser-xfs"
img_name="xfs-postmark.img"
src_root=`pwd`
source `pwd`/tests/test_common.sh
source `pwd`/tests/fs_common.sh

echo "Testing browse.."
run_test_match 4 "startoff = 0x80..0" "ext_startoff = 8388608"

echo Testing fusebrowse-xfs
fs_fuse_cmd_img xfs xfs.img "ls -la" "ls"
fs_fuse_cmd_img xfs xfs.img "ls aggr -la" "ls_aggr"
p=`cat tests/fusebrowse-xfs/xfs.img-ls.out | awk '{ print $5 " " $9; }'`
q=`cat tests/fusebrowse-xfs/xfs.img-ls_aggr.out | awk '{ print $5 " " $9; }'`
aggr_str=`echo "$p" | grep "104857600 aggr"`
if [ -z "$aggr_str" ]; then
	echo "BAD AGGR SIZE"
	echo "$p"
	exit 1
fi

aggr3_str=`echo $q | grep "26214400 3"`
if [ -z "$aggr3_str" ]; then
	echo "BAD AGGR DIRECTORY"
	echo "$q"
	exit 1
fi

exit 0
