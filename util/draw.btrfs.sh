#!/bin/bash

source util/draw.fs.sh

BTRFS_REGEX="btrfs_"

function draw_scan_btrfs
{
	draw_scan_fs btrfs "${BTRFS_REGEX}"
	for ipath in img/btrfs-*.img; do
		fname=`echo $ipath | cut -f2 -d'/'`
		draw_scan_fs_img btrfs "$fname" "$BTRFS_REGEX"
	done
}

function draw_btrfs_img
{
	draw_fs_img btrfs "$1" "${BTRFS_REGEX}"
}


if [ -z "$1" ]; then
	draw_scan_btrfs
else
#	echo WOOOOOOOOOO $1
#	draw_btrfs_img "$1"
	draw_scan_fs_img btrfs "$1" "$BTRFS_REGEX"
fi