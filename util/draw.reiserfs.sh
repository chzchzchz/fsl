#!/bin/bash

source util/draw.fs.sh

REISER_REGEX="(reiserfs_sb'|reiser_bmpblk'|internal_node|leaf_node_blk|reiserfs_indir|reiserfs_de_head|reiserfs_direct|stat|item_head|indir_blk)"
#REISER_REGEX="(reiserfs_sb'|reiser_bmpblk'|internal_node|leaf_node)"

function draw_scan_reiserfs
{
	draw_scan_fs reiserfs "${REISER_REGEX}"
	draw_scan_fs_img reiserfs "reiserfs-many.img" "$REISER_REGEX"
	draw_scan_fs_img reiserfs "reiserfs-many-blockfile.img" "$REISER_REGEX"
	draw_scan_fs_img reiserfs "reiserfs-postmark.img" "$REISER_REGEX"
	draw_scan_fs_img reiserfs "reiserfs-relocate.img" "$REISER_REGEX"
	draw_scan_fs_img reiserfs "reiserfs-relocate-problem.img" "$REISER_REGEX"
	draw_scan_fs_img reiserfs "reiserfs-defrag.img" "$REISER_REGEX"
	draw_scan_fs_img reiserfs "reiserfs-depth.img" "$REISER_REGEX"
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