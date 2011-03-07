#!/bin/bash

src_root=`pwd`
source `pwd`/tests/fs_common.sh

scanout=tests/scantool-reiserfs/reiserfs.img.out
if [ ! -f $scanout ]; then
	echo "reiserfs: Error no scanout file $scanout "
	exit -1
fi

stats=`grep stat $scanout | sort | uniq | wc -l`
if [ $stats -ne 37 ]; then
	echo "Expected stat items = 37. Got stat items = $stats"
	exit -1
fi

echo Testing fusebrowse-reiserfs
fs_fuse_cmd_img reiserfs reiserfs.img "ls -la" "ls"
#fs_fuse_cmd_img reiserfs reiserfs-postmark.img "ls -la j_header" "ls_jheader"
p=`cat tests/fusebrowse-reiserfs/reiserfs.img-ls.out | awk '{ print $5 " " $9; }'`
sb_str=`echo "$p" | grep "204 sb_new"`
if [ -z "$sb_str" ]; then
	echo "BAD SB STR"
	echo "$p"
	exit 1
fi

rawblks_str=`echo "$p" | grep "104857600 raw_blocks"`
if [ -z "$sb_str" ]; then
	echo "BAD RAWBLK BYTES"
	echo "$p"
	exit 1
fi


exit 0
