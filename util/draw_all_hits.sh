#!/bin/bash

# $1 = fs, $2 = img
function draw_hits
{
	fs=$1
	imgname=$2
	imgsz=`ls -l img/$imgname | awk '{print $5; }'`
	util/draw_hits.py tests/scantool-${fs}/${imgname}.hits $imgsz tests/scantool-${fs}/${imgname}.hits.png
	util/draw_hits.py tests/scantool-${fs}/${imgname}.misses $imgsz tests/scantool-${fs}/${imgname}.misses.png
}

start_time=`date +%s`

for fs in ext2 nilfs2 vfat iso9660; do
	cd img
	dirdat=`ls ${fs}*.img`
	cd ..
	for a in $dirdat; do
		draw_hits ${fs} "$a"
	done
done

end_time=`date +%s`

echo "Draw Hits: Total seconds: " `expr $end_time - $start_time`
