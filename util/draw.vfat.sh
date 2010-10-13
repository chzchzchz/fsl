#!/bin/bash

source util/draw.fs.sh

VFAT_REGEX="(fat'|fat_de|fat_ent|bpb|'voff' : 0.*cluster)"

function draw_scan_vfat
{
	draw_scan_fs vfat "$VFAT_REGEX"
}

function draw_vfat_img
{
	draw_fs_img vfat "$1" "${VFAT_REGEX}"
}

if [ -z "$1" ]; then
	draw_scan_vfat
else
	draw_vfat_img "$1"
fi
