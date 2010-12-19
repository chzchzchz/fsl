#!/bin/bash

# $1 = fs, $2 = img
function draw_hits
{
	fs=$1
	imgname=$2
	imgsz=`ls -l img/$imgname | awk '{print $5; }'`
	util/draw_hits.py tests/scantool-${fs}/${imgname}.hits $imgsz tests/scantool-${fs}/${imgname}.hits.png
	util/draw_hits.py tests/scantool-${fs}/${imgname}.misses $imgsz tests/scantool-${fs}/${imgname}.misses.png
	if [ -e tests/defragtool-${fs} ]; then
		util/draw_hits.py tests/defragtool-${fs}/${imgname}.hits $imgsz tests/defragtool-${fs}/${imgname}.hits.png
		util/draw_hits.py tests/defragtool-${fs}/${imgname}.misses $imgsz tests/defragtool-${fs}/${imgname}.misses.png
	fi
	if [ -e tests/relocate-${fs} ]; then
		util/draw_hits.py tests/defragtool-${fs}/${imgname}.hits $imgsz tests/relocate-${fs}/${imgname}.hits.png
		util/draw_hits.py tests/defragtool-${fs}/${imgname}.misses $imgsz tests/relocate-${fs}/${imgname}.misses.png
	fi
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
