#!/bin/bash

tool_name="browser-ext2"
img_name="ext2.img"
src_root=`pwd`
source `pwd`/tests/test_common.sh

# visit virtual file (inode 0)
run_test 1 "Virtual Visit"
run_test 2 "Virtual Dump"

echo Testing fusebrowse-ext2
${src_root}/bin/fusebrowse-ext2 img/ext2.img tmp
ls -la tmp  >tests/fusebrowse-ext2/ls.out
p=`cat tests/fusebrowse-ext2/ls.out | awk '{ print $5 " " $9; }'`
fusermount -u tmp
sb_str=`echo "$p" | grep "1024 sb"`
if [ -z "$sb_str" ]; then
	echo "BAD SB STR"
	echo "$p"
	exit 1
fi

blocks_str=`echo "$p" | grep "104857600 blocks"`
if [ -z "$blocks_str" ]; then
	echo "BAD BLOCKS STR"
	echo "$p"
	exit 1
fi

exit 0
