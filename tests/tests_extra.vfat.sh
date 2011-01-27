#!/bin/bash

fs="vfat"
source ${src_root}/tests/test_common.sh
source ${src_root}/tests/fs_common.sh

imgname=${fs}-32-many.img
fs_scan_startup_img $fs ${fs}-32-many.img

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
if [ "$fcount" -ne 27561 ]; then
	echo "BADCOUNT $fcount != 27561"
	exit -2
fi

imgname=$fs-many-4097.img
fs_scan_startup_img $fs $imgname

imgname=$fs-defrag-postmark.img
cp ${src_root}/img/$fs-postmark.img ${src_root}/img/$imgname
fs_defrag_startup_img $fs $imgname
fs_scan_startup_img $fs $imgname
fcount=`grep "file_cluster" "${src_root}"/tests/scantool-$fs/$imgname.out | wc -l`
if [ "$fcount" -ne 27561 ]; then
	echo "BADCOUNT $fcount != 27561 for ${src_root}/tests/scantool-$fs/$imgname.out"
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

imgname=$fs-manymore.img
fs_scan_startup_img $fs $imgname

imgname=$fs-reloc-greenspan.img
cp ${src_root}/img/$fs-manymore.img ${src_root}/img/$imgname
fs_reloc_img $fs $imgname ${src_root}/tests/greenspan.png.reloc
fs_scan_startup_img $fs $imgname

imgname=$fs-scatter.img
cp ${src_root}/img/$fs-postmark.img ${src_root}/img/$imgname
fs_scatter_startup_img $fs $imgname
fs_scan_startup_img $fs $imgname
fcount=`grep "file_cluster" "${src_root}"/tests/scantool-$fs/$imgname.out | wc -l`
if [ "$fcount" -ne 27561 ]; then
	echo "BADCOUNT $fcount != 27561"
	exit -2
fi

imgname=$fs-thrashcopy.img
cp ${src_root}/img/$fs-scatter.img ${src_root}/img/$imgname
fs_thrashcopy_startup_img $fs $imgname
fs_scan_startup_img $fs $imgname
fcount=`grep "file_cluster" "${src_root}"/tests/scantool-$fs/$imgname.out | wc -l`
if [ "$fcount" -ne 27561 ]; then
	echo "BADCOUNT $fcount != 27561"
	exit -2
fi

imgname=$fs-defrag.img
cp ${src_root}/img/$fs-scatter.img ${src_root}/img/$imgname
fs_defrag_startup_img $fs $imgname
fs_scan_startup_img $fs $imgname
fcount=`grep "file_cluster" "${src_root}"/tests/scantool-$fs/$imgname.out | wc -l`
if [ "$fcount" -ne 27561 ]; then
	echo "BADCOUNT $fcount != 27561 for ${src_root}/tests/scantool-$fs/$imgname.out"
	exit -2
fi

imgname=$fs-smush.img
cp ${src_root}/img/$fs-defrag.img ${src_root}/img/$imgname
fs_smush_startup_img $fs $imgname
fs_scan_startup_img $fs $imgname
fcount=`grep "file_cluster" "${src_root}"/tests/scantool-$fs/$imgname.out | wc -l`
if [ "$fcount" -ne 27561 ]; then
	echo "BADCOUNT $fcount != 27561"
	exit -2
fi
