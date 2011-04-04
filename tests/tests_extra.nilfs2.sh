#!/bin/bash

fs="nilfs2"
source ${src_root}/tests/test_common.sh
source ${src_root}/tests/fs_common.sh
source ${src_root}/tests/fsck.sh

imgname=$fs-depth.img
fsck_img $imgname
fs_scan_startup_img $fs $imgname

imgname=$fs-many.img
fsck_img $imgname
fs_scan_startup_img $fs $imgname
