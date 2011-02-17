#!/bin/bash

tool_name="browser-ext2"
img_name="ext2.img"
src_root=`pwd`
source `pwd`/tests/test_common.sh
source `pwd`/tests/fs_common.sh

# visit virtual file (inode 0)
run_test 1 "Virtual Visit"
run_test 2 "Virtual Dump"

echo Testing fusebrowse-ext2
fs_fuse_cmd_img ext2 ext2.img "ls -la" "ls"
fs_fuse_cmd_img ext2 ext2.img "ls -la grp_desc_table" "ls_grp"
fs_fuse_cmd_img ext2 ext2.img "ls -la grp_desc_table/3" "ls_grp3"
fs_fuse_cmd_img ext2 ext2.img "od -Ax -tx grp_desc_table/3/block_bmp" "od_grp3_blkbmp"
fs_fuse_cmd_img ext2 ext2.img  "ls -la grp_desc_table/3/grp_blk_bmp" "ls_grp3_blkbmp_ptr"
fs_fuse_cmd_img ext2 ext2-many.img  "ls -la grp_desc_table/2/grp_ino_tab/ino" "ls_grp2_inotab"

p=`cat tests/fusebrowse-ext2/ext2.img-ls.out | awk '{ print $5 " " $9; }'`
p_grp=`cat tests/fusebrowse-ext2/ext2.img-ls_grp.out | awk '{ print $5 " " $9; }'`
grp3=`cat tests/fusebrowse-ext2/ext2.img-ls_grp3.out | awk '{ print $5 " " $9; }'`
inotab=`cat tests/fusebrowse-ext2/ext2-many.img-ls_grp2_inotab.out | awk '{ print $5 " " $9; }'`

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

inotab_str=`echo "$inotab" | grep "128 0"`
if [ -z "$inotab_str" ]; then
	echo "Couldn't read inode_block??"
	cat tests/fusebrowse-ext2/ext2-many.img-ls_grp2_inotab.out
	exit 3
fi


od_str=`grep 00006103 tests/fusebrowse-ext2/ext2.img-od_grp3_blkbmp.out`
if [ -z "$od_str" ]; then
	echo "Failed ot read group block bitmap pointer"
	cat tests/fusebrowse-ext2/ext2.img-od_grp3_blkbmp.out
	exit 4
fi

exit 0
