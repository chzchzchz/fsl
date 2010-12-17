#!/bin/bash

fs="vfat"
source ${src_root}/tests/test_common.sh
source ${src_root}/tests/fs_common.sh

imgname=$fs-many.img
fs_scan_startup_img $fs $fs-many.img

fcount=`grep "file_cluster" "${src_root}"/tests/scantool-$fs/$fs-many.img.scan.out | wc -l`
if [ "$fcount" -ne 10100 ]; then
	echo "BADCOUNT $fcount != 10100"
	exit -2
fi

imgname=$fs-postmark.img
fs_scan_startup_img $fs $fs-postmark.img
