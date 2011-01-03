#!/bin/bash

source util/draw.fs.sh

REISER_REGEX="(reiserfs_sb'|reiser_bmpblk'|internal_node|leaf_node_blk|reiserfs_indir|reiserfs_de_head|reiserfs_direct|stat|item_head|indir_blk)"
#REISER_REGEX="(reiserfs_sb'|reiser_bmpblk'|internal_node|leaf_node)"

function draw_scan_reiserfs
{
	draw_scan_fs reiserfs "${REISER_REGEX}"
	for ipath in img/reiserfs-*.img; do
		fname=`echo $ipath | cut -f2 -d'/'`
		draw_scan_fs_img reiserfs "$fname" "${REISER_REGEX}"
	done
}

function draw_reiserfs_img
{
	draw_fs_img reiserfs "$1" "${REISER_REGEX}"
}


if [ -z "$1" ]; then
	draw_scan_reiserfs
else
	draw_reiserfs_img "$1"
fi