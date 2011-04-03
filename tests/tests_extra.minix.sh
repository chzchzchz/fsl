#!/bin/bash

fs="minix"
source ${src_root}/tests/test_common.sh
source ${src_root}/tests/fs_common.sh
source ${src_root}/tests/fsck.sh

imgname=$fs-depth.img
fsck_img $imgname
fs_scan_startup_img $fs $imgname

imgname=$fs-many.img
fsck_img $imgname
fs_scan_startup_img $fs $imgname

imgname=$fs-relocate.img
cp ${src_root}/img/$fs-many.img ${src_root}/img/$imgname
fs_reloc_startup_img $fs $imgname
fsck_img $imgname
fs_scan_startup_img $fs $imgname

do_fsck_test "printf '\xff\xff' >sb/s_log_zone_size" "chk_1k_zones" "minix-many.img"
do_fsck_test "printf '\x0\x0' >sb/s_imap_blocks" "chk_bad_s_imap_blocks" "minix-many.img"
do_fsck_test "printf '\x0\x0' >sb/s_zmap_blocks" "chk_bad_s_zmap_blocks" "minix-many.img"
