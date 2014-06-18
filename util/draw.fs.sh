#!/bin/bash

function draw_fs_img
{
	FS=$1
	FSIMG=$2
	REGEX=$3
	IMGNAME=$4
	SCANOUTFNAME="${FSIMG}.out"
	OUTPNG="${FSIMG}.png"
	DRAWLOG="${FSIMG}.log"
	SZ_X=""
	SZ_Y=""

	if [ ! -f  $SCANOUTFNAME ]; then
		echo "${FSIMG}: No files generated? Run some tests first."
		return
	fi

	ispicture=`echo "$SCANOUTFNAME" | grep "\.reloc\."`
	if [ ! -z "$ispicture" ]; then
		pic=`echo "$IMGNAME" | cut -f2,3 -d'.'`.pic
		SZ_X=`head -n1 tests/$pic`
		SZ_Y=`head -n2 tests/$pic | tail -n1`
		echo $pic $SZ_X $SZ_Y
	fi

	processed_fname="${FSIMG}.processed.out"
	grep Mode $SCANOUTFNAME | egrep "$REGEX" >${processed_fname}
	src_sz=`ls -l $IMGNAME  | awk '{ print $5; }'`

	if [ -z "$SZ_X" ]; then
		./util/draw_scan.py "${processed_fname}" $src_sz "$OUTPNG" >"$DRAWLOG"
	else
		./util/draw_scan.py "${processed_fname}" $src_sz "$OUTPNG" "$SZ_X" "$SZ_Y" >"$DRAWLOG"
	fi
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
