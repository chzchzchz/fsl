#!/bin/bash

fs="iso9660"
source ${src_root}/tests/test_common.sh
source ${src_root}/tests/fs_common.sh

imgname=$fs-depth.img
fs_scan_startup_img $fs $imgname

imgname=$fs-many.img
fs_scan_startup_img $fs $fs-many.img

imgname=$fs-postmark.img
fs_scan_startup_img $fs $imgname
