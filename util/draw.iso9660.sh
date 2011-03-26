#!/bin/bash

source util/draw.fs.sh

ISO9660_REGEX="(iso_record|iso_vol_desc)"

function draw_scan_iso9660
{
	draw_scan_fs iso9660 "${ISO9660_REGEX}"
	for ipath in img/iso9660-*.img; do
		fname=`echo $ipath | cut -f2 -d'/'`
		draw_scan_fs_img iso9660 "$fname" "$ISO9660_REGEX"
	done
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