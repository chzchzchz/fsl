#!/bin/bash

# $1 = fs, $2 = img
function draw_hits
{
	fs=$1
	imgname=$2
	imgsz=`ls -l img/$imgname | awk '{print $5; }'`
	util/draw_hits.py tests/scantool-${fs}/${imgname}.hits $imgsz tests/scantool-${fs}/${imgname}.hits.png
}

for fs in ext2 nilfs2 vfat; do
	cd img
	dirdat=`ls ${fs}*.img`
	cd ..
	for a in $dirdat; do
		draw_hits ${fs} "$a"
	done
done
