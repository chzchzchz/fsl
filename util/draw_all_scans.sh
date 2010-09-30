#!/bin/bash

# as if run from src_root

EXT2_REGEX="(dir|sb|group_desc|bmp|ext2_inode'|ext2_data_block)"
VFAT_REGEX="(fat'|fat_de|fat_ent|bpb|'voff' : 0.*cluster)"
NILFS2_REGEX="(nilfs_sb|nilfs_finfo|nilfs_block)"

function draw_fs_img
{
	FS=$1
	FSIMG=$2
	REGEX=$3
	SCANOUTFNAME="tests/scantool-$FS/${FSIMG}.scan.out"
	OUTPNG="tests/scantool-$FS/${FSIMG}.png"
	DRAWLOG="tests/scantool-$FS/${FSIMG}.draw.log"
	if [ ! -f  $SCANOUTFNAME ]; then
		echo "No files generated? Run some tests first."
		exit -1
	fi

	processed_fname="tests/scantool-${FS}/${FSIMG}.scan.processed.out"
	grep Mode $SCANOUTFNAME | egrep "$REGEX" >${processed_fname}
	src_sz=`ls -l img/$FSIMG  | awk '{ print $5; }'`

	./util/draw_scan.py "${processed_fname}" $src_sz "$OUTPNG" >"$DRAWLOG"

}

function draw_fs
{
	FS=$1
	REGEX=$2
	draw_fs_img "$FS" "${FS}.img" "$REGEX"
}

function draw_ext2
{
	draw_fs ext2 "$EXT2_REGEX"
}


function draw_vfat
{
	draw_fs vfat "$VFAT_REGEX"
}

function draw_nilfs2
{
	draw_fs nilfs2 "$NILFS2_REGEX"
}

draw_ext2
draw_vfat
draw_nilfs2
draw_fs_img "ext2" "ext2-small.img" "$EXT2_REGEX"
