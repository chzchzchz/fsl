#!/bin/bash

tool_name="browser-vfat"
img_name="vfat.img"
src_root=`pwd`
source `pwd`/tests/test_common.sh
source `pwd`/tests/fs_common.sh

run_test 1 "Visit Root DE[1]"
run_test_no_match 2 "Virt-If. Non-File" "vfile"
run_test_match 3 "Virt-If. File." "vfile"

# don't get boot sector!!
run_test_no_match 4 "Virtual. Read file xlate." "55 aa"

echo Testing fusebrowse-vfat
fs_fuse_cmd_img vfat vfat.img "ls -la" "ls"
fs_fuse_cmd_img vfat vfat.img "ls -la d_bpb" "ls_bpb"
fs_fuse_cmd_img vfat vfat.img "ls -la root_dir" "ls_rootdir"
fs_fuse_cmd_img vfat vfat-postmark.img 'ls -la root_dir/S3*' 'ls_rootdir_s3'

p=`cat tests/fusebrowse-vfat/vfat.img-ls.out | awk '{ print $5 " " $9; }'`
p_bpb=`cat tests/fusebrowse-vfat/vfat.img-ls_bpb.out | awk '{ print $5 " " $9; }'`


sb_str=`echo "$p" | grep "62 d_bpb"`
if [ -z "$sb_str" ]; then
	echo "BAD BPB"
	echo "$p"
	exit 1
fi

fats_str=`echo $p | grep "204800 fats"`
if [ -z "$fats_str" ]; then
	echo "BAD FAT LENGTH"
	echo "$p"
	exit 2
fi

rootdir=`cat tests/fusebrowse-vfat/vfat.img-ls.out | awk '{ print $1 " " $5 " " $9; }'`
rootde_str=`echo $rootdir | grep "dr-x------ 16384 root_dir"`
if [ -z "$rootde_str" ]; then
	echo "NO ROOT FAT DE"
	echo "$p"
	exit 3
fi

rootdirs=`cat tests/fusebrowse-vfat/vfat.img-ls_rootdir.out | awk '{ print $1 " " $5 " " $9; }'`
rootdirls_str=`echo $rootdirs | grep "dr-x------ 32 4D"`
if [ -z "$rootdirls_str" ]; then
	echo "NO ROOTDIR LISTING?"
	echo "$rootdirs"
	exit 3
fi


rootdir_s3=`cat tests/fusebrowse-vfat/vfat-postmark.img-ls_rootdir_s3.out | awk '{ print $1 " " $5 " " $9; }'`
rootdir_s3_str=`echo $rootdir_s3 | grep vdir`
if [ -z "$rootdir_s3_str" ] ; then
	echo "NO S3 VDIR!!!"
	echo "$rootdir_s3"
	exit 4
fi
