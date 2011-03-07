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
fs_fuse_cmd_img ext2 ext2.img  "ls -la root_ino/vdir/5.aaa/ent_ino/0/vfile/0" "ls_vfile"
fs_fuse_cmd_img ext2 ext2-many.img  "ls -la root_ino/vdir/" "ls_rootino"
fs_fuse_cmd_img ext2 ext2-many.img  "ls -la root_ino/vdir/1d" "ls_rootino_1d"
fs_fuse_cmd_img ext2 ext2.img  "ls -la grp_desc_table/3/grp_blk_bmp" "ls_grp3_blkbmp_ptr"
fs_fuse_cmd_img ext2 ext2.img  "ls -la grp_desc_table/3/grp_blk_bmp" "ls_grp3_blkbmp_ptr"
fs_fuse_cmd_img ext2 ext2.img 'ls -la grp_desc_table/8/grp_ino_bmp/0/' 'ls_grp8_inobmp'
fs_fuse_failcmd_img ext2 ext2.img "stat blocks/.htaccess" "stat_blocks_bogus"
fs_fuse_failcmd_img ext2 ext2.img "ls -la root_ino/0/direct/.htaccess" "ls_direct_htaccess_bogus"
#fs_fuse_cmd_img ext2 ext2.img  "ls -la root_ino/vdir" "ls_grp3_blkbmp_ptr"

p=`cat tests/fusebrowse-ext2/ext2.img-ls.out | awk '{ print $5 " " $9; }'`
p_grp=`cat tests/fusebrowse-ext2/ext2.img-ls_grp.out | awk '{ print $5 " " $9; }'`
grp3=`cat tests/fusebrowse-ext2/ext2.img-ls_grp3.out | awk '{ print $5 " " $9; }'`
rootino=`cat tests/fusebrowse-ext2/ext2-many.img-ls_rootino.out | awk '{ print $5 " " $9; }'`
rootino_1d=`cat tests/fusebrowse-ext2/ext2-many.img-ls_rootino_1d.out | awk '{ print $5 " " $9; }'`
vfile=`cat tests/fusebrowse-ext2/ext2.img-ls_vfile.out | awk '{ print $5 " " $9; }'`
grp8_inobmp=`cat tests/fusebrowse-ext2/ext2.img-ls_grp8_inobmp.out | awk '{ print $1 " " $5 " " $9; }'`

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

vfile_str=`echo "$vfile" | grep "1024 data"`
if [ -z "$vfile_str" ]; then
	echo "BAD VFILE STR"
	echo $vfile_str
	exit 5
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

rootino_str=`echo "$rootino" | grep "20 lost+found"`
if [ -z "$rootino_str" ]; then
	echo "Couldn't read root_ino vdir??"
	cat tests/fusebrowse-ext2/ext2-many.img-ls_rootino.out
	exit 3
fi

rootino_1d_str=`echo "${rootino_1d}" | grep "1 file_type"`
if [ -z "$rootino_1d_str" ]; then
	echo "Couldn't read directory entry for /1d/??"
	cat tests/fusebrowse-ext2/ext2-many.img-ls_rootino_1d.out
	exit 3
fi


od_str=`grep 00006103 tests/fusebrowse-ext2/ext2.img-od_grp3_blkbmp.out`
if [ -z "$od_str" ]; then
	echo "Failed ot read group block bitmap pointer"
	cat tests/fusebrowse-ext2/ext2.img-od_grp3_blkbmp.out
	exit 4
fi

grp8_inobmp_str=`echo "$grp8_inobmp" | grep '\-rw------- 1024 data'`
if [ -z "$grp8_inobmp_str" ]; then
	echo "Failed to get data file in inobmp"
	cat tests/fusebrowse-ext2/ext2.img-ls_grp8_inobmp.out
	exit 5
fi

exit 0
