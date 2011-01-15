#!/bin/bash


if [ -z $FILESYSTEM ]; then
	echo "Linux-4gb: no fs given."
	exit -1
fi
fs=$FILESYSTEM

src_root=`pwd`
source ${src_root}/tests/test_common.sh
source ${src_root}/tests/fs_common.sh
source ${src_root}/tests/fsck.sh

imgname=$fs-linux-4gb.img
fs_scan_startup_img $fs $imgname

imgname=$fs-linux-4gb-relocate.img
cp ${src_root}/img/$fs-linux-4gb.img ${src_root}/img/$imgname
fs_reloc_startup_img $fs $imgname
fsck_img $imgname
fs_scan_startup_img $fs $imgname

imgname=$fs-linux-4gb-scatter.img
cp ${src_root}/img/$fs-linux-4gb-relocate.img ${src_root}/img/$imgname
fs_scatter_startup_img $fs $imgname
fsck_img $imgname
fs_scan_startup_img $fs $imgname


imgname=$fs-linux-4gb-defrag.img
cp ${src_root}/img/$fs-linux-4gb-scatter.img ${src_root}/img/$imgname
fs_defrag_startup_img $fs $imgname
fsck_img $imgname
fs_scan_startup_img $fs $imgname


imgname=$fs-linux-4gb-smush.img
cp ${src_root}/img/$fs-linux-4gb-defrag.img ${src_root}/img/$imgname
fs_smush_startup_img $fs $imgname
fsck_img $imgname
fs_scan_startup_img $fs $imgname