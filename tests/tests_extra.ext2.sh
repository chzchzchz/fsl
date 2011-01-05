#!/bin/bash

fs="ext2"
source ${src_root}/tests/test_common.sh
source ${src_root}/tests/fs_common.sh

function fsck_img
{
	LODEV=/dev/loop1 ${src_root}/img/loopback_fs_setup.sh ${src_root}/img/$imgname ext2
	sudo /sbin/fsck -v -f -n /dev/loop1 2>&1  >${src_root}/fsck.fail.$fs.stdout
	retval=$?
	LODEV=/dev/loop1 ${src_root}/img/loopback_fs_teardown.sh
	if [ $retval -ne 0 ]; then
		echo "FSCK FAILED ON $imgname."
		exit $retval
	fi
}

imgname=$fs-depth.img
fsck_img
fs_scan_startup_img $fs $imgname

imgname=$fs-many.img
fsck_img
fs_scan_startup_img $fs $imgname

imgname=$fs-postmark.img
fsck_img
fs_scan_startup_img $fs $imgname

imgname=$fs-relocate.img
cp ${src_root}/img/$fs-many.img ${src_root}/img/$imgname
fs_reloc_startup_img $fs $imgname
fsck_img
fs_scan_startup_img $fs $imgname

imgname=$fs-defrag.img
cp ${src_root}/img/$fs-postmark.img ${src_root}/img/$imgname
fs_defrag_startup_img $fs $imgname
fsck_img
fs_scan_startup_img $fs $imgname

imgname=$fs-scatter.img
cp ${src_root}/img/$fs-postmark.img ${src_root}/img/$imgname
fs_scatter_startup_img $fs $imgname
fsck_img
fs_scan_startup_img $fs $imgname

imgname=$fs-smush.img
cp ${src_root}/img/$fs-postmark.img ${src_root}/img/$imgname
fs_smush_startup_img $fs $imgname
fsck_img
fs_scan_startup_img $fs $imgname
