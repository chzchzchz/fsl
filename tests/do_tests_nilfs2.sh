#!/bin/bash

tool_name="browser-nilfs2"
img_name="nilfs2-many.img"
src_root=`pwd`
source `pwd`/tests/test_common.sh
source `pwd`/tests/fs_common.sh

echo "Testing NILFS2 Fuse"

fs_fuse_cmd_img nilfs2 nilfs2-many.img "ls -la" "ls"
fs_fuse_cmd_img nilfs2 nilfs2-many.img "ls -la segs/8" "ls_seg8"
fs_fuse_failcmd_img nilfs2 nilfs2-many.img "ls -la segs/9" "ls_seg9"

p=`cat tests/fusebrowse-nilfs2/nilfs2-many.img-ls.out | awk '{ print $5 " " $9; }'`
p_seg8=`cat tests/fusebrowse-nilfs2/nilfs2-many.img-ls_seg8.out | awk '{ print $5 " " $9; }'`

p_chk_nsb2=`echo $p | grep "1024 nsb2"`
if [ -z "$p_chk_nsb2" ]; then
	echo "BAD NSB2"
	echo "$p"
	exit 1
fi

p_chk_nsb=`echo $p | grep "1024 nsb"`
if [ -z "$p_chk_nsb" ]; then
	echo "BAD NSB"
	echo "$p"
	exit 2
fi

pseg8_chk_segsum=`echo "$p_seg8" | grep "12288 seg_sum"`
if [ -z "$pseg8_chk_segsum" ]; then
	echo "BAD SEGSUM"
	echo "$p_seg8"
	exit 3
fi
