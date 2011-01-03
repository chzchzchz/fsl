#!/bin/bash

fs="vfat"
source ${src_root}/tests/test_common.sh
source ${src_root}/tests/fs_common.sh

imgname=$fs-depth.img
fs_scan_startup_img $fs $imgname

imgname=$fs-many.img
fs_scan_startup_img $fs $fs-many.img

fcount=`grep "file_cluster" "${src_root}"/tests/scantool-$fs/$imgname.out | wc -l`
if [ "$fcount" -ne 10100 ]; then
	echo "BADCOUNT $fcount != 10100"
	exit -2
fi

imgname=$fs-postmark.img
fs_scan_startup_img $fs $imgname
fcount=`grep "file_cluster" "${src_root}"/tests/scantool-$fs/$imgname.out | wc -l`
if [ "$fcount" -ne 23040 ]; then
	echo "BADCOUNT $fcount != 23040"
	exit -2
fi

imgname=$fs-relocate.img
cp ${src_root}/img/$fs-many.img ${src_root}/img/$imgname
fs_reloc_startup_img $fs $imgname
fs_scan_startup_img $fs $imgname
fcount=`grep "file_cluster" "${src_root}"/tests/scantool-$fs/$imgname.out | wc -l`
if [ "$fcount" -ne 10100 ]; then
	echo "BADCOUNT $fcount != 10100"
	exit -2
fi


imgname=$fs-defrag.img
cp ${src_root}/img/$fs-postmark.img ${src_root}/img/$imgname
fs_defrag_startup_img $fs $imgname
fs_scan_startup_img $fs $imgname
fcount=`grep "file_cluster" "${src_root}"/tests/scantool-$fs/$imgname.out | wc -l`
if [ "$fcount" -ne 23040 ]; then
	echo "BADCOUNT $fcount != 23040 for ${src_root}/tests/scantool-$fs/$imgname.out"
	exit -2
fi

imgname=$fs-scatter.img
cp ${src_root}/img/$fs-postmark.img ${src_root}/img/$imgname
fs_scatter_startup_img $fs $imgname
fs_scan_startup_img $fs $imgname
fcount=`grep "file_cluster" "${src_root}"/tests/scantool-$fs/$imgname.out | wc -l`
if [ "$fcount" -ne 23040 ]; then
	echo "BADCOUNT $fcount != 23040"
	exit -2
fi


imgname=$fs-smush.img
cp ${src_root}/img/$fs-postmark.img ${src_root}/img/$imgname
fs_smush_startup_img $fs $imgname
fs_scan_startup_img $fs $imgname
fcount=`grep "file_cluster" "${src_root}"/tests/scantool-$fs/$imgname.out | wc -l`
if [ "$fcount" -ne 23040 ]; then
	echo "BADCOUNT $fcount != 23040"
	exit -2
fi
