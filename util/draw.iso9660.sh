#!/bin/bash

source util/draw.fs.sh

ISO9660_REGEX="(iso_record|iso_vol_desc)"

function draw_scan_iso9660
{
	draw_scan_fs iso9660 "${ISO9660_REGEX}"
#	draw_scan_fs_img iso9660 "iso9660-many.img" "$ISO9660_REGEX"
#	draw_scan_fs_img iso9660 "iso9660-postmark.img" "$ISO9660_REGEX"
}

function draw_iso9660_img
{
	draw_fs_img iso9660 "$1" "${ISO9660_REGEX}"
}


if [ -z "$1" ]; then
	draw_scan_iso9660
else
	draw_iso9660_img "$1"
fi