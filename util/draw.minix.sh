#!/bin/bash

source util/draw.fs.sh

MINIX_REGEX="(_inode|super_block|dir_entry|omap|zmap|file_block)"

function draw_scan_minix
{
	draw_scan_fs minix "${MINIX_REGEX}"
	for ipath in img/minix-*.img; do
		fname=`echo $ipath | cut -f2 -d'/'`
		draw_scan_fs_img minix "$fname" "$MINIX_REGEX"
	done
}

function draw_minix_img
{
	draw_fs_img minix "$1" "${MINIX_REGEX}"
}

if [ -z "$1" ]; then
	draw_scan_minix
else
	draw_minix_img "$1"
fi