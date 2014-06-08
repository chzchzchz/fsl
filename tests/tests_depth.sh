#!/bin/bash
export USE_STATS="YES"
export XFM_PIN_STACK="YES"

src_root=`pwd`

source ${src_root}/tests/test_common.sh
source ${src_root}/tests/fs_common.sh

for fs in vfat ext2 reiserfs nilfs2 minix; do
	for dnum in 1 4 16 64 128 192 256; do
		imgname=$fs-depth-$dnum.img
		fs_scan_startup_img $fs $imgname
	done
done