#!/bin/bash

# as if run from src_root

function draw_fs
{
	FS=$1
	REGEX=$2
	SCANOUTFNAME=tests/scantool-$FS/$FS.scan.out
	if [ ! -f  $SCANOUTFNAME ]; then
		echo "No files generated? Run some tests first."
		exit -1
	fi

	processed_fname="tests/scantool-${FS}/${FS}.scan.processed.out"
	grep Mode $SCANOUTFNAME | egrep "$REGEX" >${processed_fname}
	src_sz=`ls -l img/$FS.img  | awk '{ print $5; }'`

	./util/draw_scan.py "${processed_fname}" $src_sz tests/scantool-$FS/$FS.png >tests/scantool-$FS/$FS.draw.log

}

function draw_ext2
{
	draw_fs ext2 "(dir|sb|group_desc|bmp|ext2_inode'|ext2_data_block)"
}


function draw_vfat
{
	draw_fs vfat "(fat'|fat_de|fat_ent|bpb|'voff' : 0.*cluster)"
}

draw_ext2
draw_vfat
