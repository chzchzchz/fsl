#!/bin/bash

function draw_fs_img
{
	FS=$1
	FSIMG=$2
	REGEX=$3
	IMGNAME=$4
	SCANOUTFNAME="${FSIMG}.scan.out"
	OUTPNG="${FSIMG}.scan.png"
	DRAWLOG="${FSIMG}.scan.log"
	if [ ! -f  $SCANOUTFNAME ]; then
		echo "${FSIMG}: No files generated? Run some tests first."
		exit -1
	fi

	processed_fname="${FSIMG}.scan.processed.out"
	grep Mode $SCANOUTFNAME | egrep "$REGEX" >${processed_fname}
	src_sz=`ls -l $IMGNAME  | awk '{ print $5; }'`

	./util/draw_scan.py "${processed_fname}" $src_sz "$OUTPNG" >"$DRAWLOG"

}

function draw_scan_fs
{
	FS=$1
	REGEX=$2
	DATAPATH="tests/scantool-${FS}/${FS}.img"
	IMGNAME="img/${FS}.img"
	draw_fs_img "$FS" "$DATAPATH" "$REGEX" "$IMGNAME"
}

function draw_scan_fs_img
{
	FS=$1
	REGEX=$3
	IMGNAME=$2
	DATAPATH="tests/scantool-${FS}/${IMGNAME}"
	draw_fs_img "$FS" "$DATAPATH" "$REGEX" "img/$IMGNAME"
}
