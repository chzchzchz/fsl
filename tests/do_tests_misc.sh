#!/bin/bash

src_root=`pwd`
source `pwd`/tests/test_common.sh
source `pwd`/tests/fs_common.sh

MISC_FSNAMES="vfat ext2 reiserfs iso9660"

function iso9660_scan
{
	sudo mount $1 /mnt/FSTEST -o loop
	du /mnt/FSTEST >/dev/null
	sudo umount /mnt/FSTEST
}

function drop_caches
{
	sync
	echo 1 | sudo tee /proc/sys/vm/drop_caches
}

function get_native_scan_cmd
{
	case "$1" in
	iso9660)	echo "iso9660_scan"		;;
	vfat)		echo "/sbin/dosfsck"		;;
	ext2)		echo "/sbin/e2fsck -f -n "	;;
	reiserfs)	echo "/sbin/debugreiserfs -d "	;;
	*) 		exit 1				;;
	esac
}

img/loopback_fs_teardown.sh

# scan bench
# compare the performance of scanning in fsl vs native implementations
#remember to flush caches@!!!
for fs in ${MISC_FSNAMES}; do
	scancmd=`get_native_scan_cmd $fs`
	imgname=${src_root}/img/$fs-linux-4gb.img
	echo "Native scan $fs..."
	drop_caches >/dev/null
	{ time eval $scancmd "${src_root}/img/$fs-linux-4gb.img" >/dev/null ; }  2>"${src_root}/tests/misc/$fs-native.time"
	drop_caches >/dev/null
	echo "FSL scan $fs"
	{ time bin/scantool-$fs "${src_root}/img/$fs-linux-4gb.img"  >/dev/null 2>/dev/null; }  2>"${src_root}/tests/misc/$fs-fsl.time"
done