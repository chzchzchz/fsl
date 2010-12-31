#!/bin/bash

fs="ext2"
source ${src_root}/tests/test_common.sh
source ${src_root}/tests/fs_common.sh

imgname=$fs-depth.img
fs_scan_startup_img $fs $imgname

imgname=$fs-many.img
fs_scan_startup_img $fs $imgname
imgname=$fs-postmark.img
fs_scan_startup_img $fs $imgname

imgname=$fs-relocate.img
cp ${src_root}/img/$fs-many.img ${src_root}/img/$imgname
fs_reloc_startup_img $fs $imgname
fs_scan_startup_img $fs $imgname

imgname=$fs-defrag.img
cp ${src_root}/img/$fs-postmark.img ${src_root}/img/$imgname
fs_defrag_startup_img $fs $imgname
fs_scan_startup_img $fs $imgname

imgname=$fs-scatter.img
cp ${src_root}/img/$fs-postmark.img ${src_root}/img/$imgname
fs_scatter_startup_img $fs $imgname
fs_scan_startup_img $fs $imgname
