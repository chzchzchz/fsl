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
ret_p=$?
ls -la tmp/grp_desc_table >tests/fusebrowse-ext2/ls_grp.out
ret_pgrp=$?
ls -la tmp/grp_desc_table/3 >tests/fusebrowse-ext2/ls_grp3.out
ret_grp3=$?
od -Ax -tx tmp/grp_desc_table/3/block_bmp >tests/fusebrowse-ext2/od_grp3_blkbmp.out
ret_odgrp3=$?

p=`cat tests/fusebrowse-ext2/ls.out | awk '{ print $5 " " $9; }'`
p_grp=`cat tests/fusebrowse-ext2/ls_grp.out | awk '{ print $5 " " $9; }'`
grp3=`cat tests/fusebrowse-ext2/ls_grp3.out | awk '{ print $5 " " $9; }'`
fusermount -u tmp

if [ $ret_p -ne 0 ]; then
	exit $ret_p
fi
if [ $ret_pgrp -ne 0 ]; then
	exit $ret_pgrp
fi
if [ $ret_grp3 -ne 0 ]; then
	exit $ret_grp3
fi

if [ $ret_odgrp3 -ne 0 ]; then
	exit $ret_odgrp3
fi

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

grp_str=`echo "$p_grp" | grep "32 2"`
if [ -z "$grp_str" ]; then
	echo "No group directory!"
	echo "$p_grp"
	exit 2
fi

grp3_str=`echo "$grp3" | grep "4 block_bmp"`
if [ -z "$grp3_str" ]; then
	echo "No block_bmp in group directory!"
	echo "$grp3"
	exit 3
fi

od_str=`grep 00006103 tests/fusebrowse-ext2/od_grp3_blkbmp.out`
if [ -z "$od_str" ]; then
	echo "Failed ot read group block bitmap pointer"
	cat tests/fusebrowse-ext2/od_grp3_blkbmp.out
	exit 4
fi

exit 0
