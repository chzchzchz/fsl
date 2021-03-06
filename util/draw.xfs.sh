#!/bin/bash

source util/draw.fs.sh

XFS_REGEX="(xfs_sb|xfs_agf|xfs_agi|xfs_agfl|xfs_freelist_blk|xfs_ino_btree|xfs_bno_blk|xfs_cnt_blk|xfs_dinode[^_]|xfs_block|xfs_dir2_sf)"

function draw_scan_xfs
{
	draw_scan_fs xfs "${XFS_REGEX}"
	for ipath in img/xfs-*.img; do
		fname=`echo $ipath | cut -f2 -d'/'`
		draw_scan_fs_img xfs "$fname" "${XFS_REGEX}"
	done
}

function draw_xfs_img
{
	draw_fs_img xfs "$1" "${XFS_REGEX}"
}


if [ -z "$1" ]; then
	draw_scan_xfs
else
	draw_xfs_img "$1"
fi