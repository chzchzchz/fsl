#!/bin/bash

source util/draw.fs.sh

NILFS2_REGEX="(nilfs_sb|nilfs_finfo|nilfs_block)"

function draw_scan_nilfs2
{
	draw_scan_fs nilfs2 "${NILFS2_REGEX}"
}

function draw_nilfs2_img
{
	draw_fs_img nilfs2 "$1" "${NILFS2_REGEX}"
}

if [ -z "$1" ]; then
	draw_scan_nilfs2
else
	draw_nilfs2_img "$1"
fi
