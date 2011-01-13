#!/bin/bash

fs="reiserfs"
source ${src_root}/tests/test_common.sh
source ${src_root}/tests/fs_common.sh

imgname=$fs-depth.img
fs_scan_startup_img $fs $imgname
statcount=`grep "stat_" ${src_root}/tests/scantool-$fs/$imgname.out | sort | uniq | wc -l`
if [ $statcount -ne 302 ]; then
	echo "Expected stat items = 302. Got stat items = $statcount"
	exit -1
fi

imgname=$fs-many.img
fs_scan_startup_img $fs $imgname
statcount=`grep "stat_" ${src_root}/tests/scantool-$fs/$imgname.out | sort | uniq | wc -l`
# NOTE HERE: WE ONLY COUNT NON-DELETED NODES
if [ $statcount -ne 10202 ]; then
	echo "Expected stat items = 10202. Got stat items = $statcount"
	exit -1
fi

imgname=$fs-postmark.img
fs_scan_startup_img $fs $imgname
statcount=`grep "stat_" ${src_root}/tests/scantool-$fs/$imgname.out | sort | uniq | wc -l`
# NOTE HERE: WE ONLY COUNT NON-DELETED NODES
if [ $statcount -ne 5007 ]; then
	echo "Expected stat items = 5007. Got stat items = $statcount"
	exit -1
fi

imgname=$fs-many-blockfile.img
fs_scan_startup_img $fs $imgname

imgname=$fs-relocate.img
cp ${src_root}/img/$fs-many-blockfile.img ${src_root}/img/$imgname
fs_reloc_startup_img $fs $imgname
fs_scan_startup_img $fs $imgname

#imgname=$fs-relocate-problem.img
#cp ${src_root}/img/$fs-postmark.img ${src_root}/img/$imgname
#fs_reloc_img $fs $imgname "${src_root}/tests/reloc.problem.pic"
#fs_scan_startup_img $fs $imgname

imgname=$fs-scatter.img
cp ${src_root}/img/$fs-postmark.img ${src_root}/img/$imgname
fs_scatter_startup_img $fs $imgname
fs_scan_startup_img $fs $imgname
statcount=`grep "stat_" ${src_root}/tests/scantool-$fs/$imgname.out | sort | uniq | wc -l`
# NOTE HERE: WE ONLY COUNT NON-DELETED NODES
if [ $statcount -ne 5007 ]; then
	echo "Expected stat items = 5007. Got stat items = $statcount"
	exit -1
fi

imgname=$fs-smush.img
cp ${src_root}/img/$fs-postmark.img ${src_root}/img/$imgname
fs_smush_startup_img $fs $imgname
fs_scan_startup_img $fs $imgname

imgname=$fs-defrag.img
cp ${src_root}/img/$fs-postmark.img ${src_root}/img/$imgname
fs_defrag_startup_img $fs $imgname
fs_scan_startup_img $fs $imgname
