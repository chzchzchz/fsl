#!/bin/bash

fs="ext2"
source ${src_root}/tests/test_common.sh
source ${src_root}/tests/fs_common.sh
source ${src_root}/tests/fsck.sh

imgname=$fs-depth.img
fsck_img $imgname
fs_scan_startup_img $fs $imgname

imgname=$fs-many.img
fsck_img $imgname
fs_scan_startup_img $fs $imgname

imgname=$fs-postmark.img
fsck_img $imgname
fs_scan_startup_img $fs $imgname

imgname=$fs-relocate.img
cp ${src_root}/img/$fs-many.img ${src_root}/img/$imgname
fs_reloc_startup_img $fs $imgname
fsck_img $imgname
fs_scan_startup_img $fs $imgname

imgname=$fs-scatter.img
cp ${src_root}/img/$fs-postmark.img ${src_root}/img/$imgname
fs_scatter_startup_img $fs $imgname
fsck_img $imgname
fs_scan_startup_img $fs $imgname

imgname=$fs-thrashcopy.img
cp ${src_root}/img/$fs-scatter.img ${src_root}/img/$imgname
fs_thrashcopy_startup_img $fs $imgname
fsck_img $imgname
fs_scan_startup_img $fs $imgname

imgname=$fs-defrag.img
cp ${src_root}/img/$fs-scatter.img ${src_root}/img/$imgname
fs_defrag_startup_img $fs $imgname
fsck_img $imgname
fs_scan_startup_img $fs $imgname

imgname=$fs-smush.img
cp ${src_root}/img/$fs-defrag.img ${src_root}/img/$imgname
fs_smush_startup_img $fs $imgname
fsck_img $imgname
fs_scan_startup_img $fs $imgname

#fsck tests
echo $fs-FSCK Tests

do_fsck_test "printf '\xff\xff\xff\xff' >sb/s_first_data_block" "chk_first_dblk"
do_fsck_test "printf '\xff\xff\xff\xff' >sb/s_inodes_c" "chk_inode_count"
