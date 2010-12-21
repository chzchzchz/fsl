#!/bin/bash

source util/draw.fs.sh

EXT2_REGEX="(dir|sb|group_desc|bmp|ext2_inode'|ext2_data_block)"

function draw_scan_ext2
{
	draw_scan_fs ext2 "${EXT2_REGEX}"
	draw_scan_fs_img ext2 "ext2-small.img" "$EXT2_REGEX"
	draw_scan_fs_img ext2 "ext2-many.img" "$EXT2_REGEX"
	draw_scan_fs_img ext2 "ext2-postmark.img" "$EXT2_REGEX"
	draw_scan_fs_img ext2 "ext2-relocate.img" "$EXT2_REGEX"
	draw_scan_fs_img ext2 "ext2-defrag.img" "$EXT2_REGEX"
	draw_scan_fs_img ext2 "ext2-depth.img" "$EXT2_REGEX"
}

function draw_ext2_img
{
	draw_fs_img ext2 "$1" "${EXT2_REGEX}"
}


if [ -z "$1" ]; then
	draw_scan_ext2
else
	draw_ext2_img "$1"
fi