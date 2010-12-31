#!/bin/bash

# $1 = fs, $2 = img
function draw_hits
{
	fs=$1
	imgname=$2
	imgsz=`ls -l img/$imgname | awk '{print $5; }'`
	util/draw_hits.py tests/scantool-${fs}/${imgname}.hits $imgsz tests/scantool-${fs}/${imgname}.hits.png
	util/draw_hits.py tests/scantool-${fs}/${imgname}.misses $imgsz tests/scantool-${fs}/${imgname}.misses.png
	if [ -e tests/defragtool-${fs}/${imgname}.hits ]; then
		util/draw_hits.py tests/defragtool-${fs}/${imgname}.hits $imgsz tests/defragtool-${fs}/${imgname}.hits.png
		util/draw_hits.py tests/defragtool-${fs}/${imgname}.misses $imgsz tests/defragtool-${fs}/${imgname}.misses.png
		util/draw_hits.py tests/defragtool-${fs}/${imgname}.writes $imgsz tests/defragtool-${fs}/${imgname}.writes.png
	fi
	if [ -e tests/relocate-${fs}/${imgname}.hits ]; then
		util/draw_hits.py tests/relocate-${fs}/${imgname}.hits $imgsz tests/relocate-${fs}/${imgname}.hits.png
		util/draw_hits.py tests/relocate-${fs}/${imgname}.misses $imgsz tests/relocate-${fs}/${imgname}.misses.png
		util/draw_hits.py tests/relocate-${fs}/${imgname}.writes $imgsz tests/relocate-${fs}/${imgname}.writes.png
	fi
	if [ -e tests/scattertool-${fs}/${imgname}.hits ]; then
		util/draw_hits.py tests/scattertool-${fs}/${imgname}.hits $imgsz tests/scattertool-${fs}/${imgname}.hits.png
		util/draw_hits.py tests/scattertool-${fs}/${imgname}.misses $imgsz tests/scattertool-${fs}/${imgname}.misses.png
		util/draw_hits.py tests/scattertool-${fs}/${imgname}.writes $imgsz tests/scattertool-${fs}/${imgname}.writes.png
	fi

}

start_time=`date +%s`

for fs in ext2 nilfs2 vfat iso9660 reiserfs; do
	cd img
	dirdat=`ls ${fs}*.img`
	cd ..
	for a in $dirdat; do
		draw_hits ${fs} "$a"
	done
done

end_time=`date +%s`

echo "Draw Hits: Total seconds: " `expr $end_time - $start_time`
