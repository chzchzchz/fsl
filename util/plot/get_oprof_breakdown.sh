#!/bin/bash
#!/bin/bash

source "util/plot/config.sh"

function get_display
{
	if [ $fs = 'vfat' ]; then
		displayname="VFAT"
		return
	fi
	if [ $fs = 'reiserfs' ]; then
		displayname='ReiserFS'
		return
	fi
	displayname=$fs
}

function printstats
{
	if [ ! -e $fname ]; then
#		echo "COULD NOT GET $fname"
		return
	fi
	get_display

	AWKSTR="BEGIN { x = 0 } { x += \$3 } END { print x }"
	displayname=${displayname}-$tooltype
	fslcodefrac=`grep types.s $fname | awk "$AWKSTR"`
	virtfrac=`grep virt $fname | awk "$AWKSTR"`
	iofrac=`egrep "(io.c|cache.c)" $fname | awk "$AWKSTR"`
	typefrac=`egrep "(type_info.c|type_print.c)" $fname | awk "$AWKSTR"`
	typetotfrac="$typefrac"
	slopfrac=`bc -l <<< "100 - ($fslcodefrac + $virtfrac + $iofrac + $typetotfrac)"`
	echo $displayname $iofrac $typetotfrac $virtfrac $fslcodefrac $slopfrac
}

function header
{
	echo Filesystem \"I/O Ops\" Types Xlate FS Other
}

header
for fs in iso9660 vfat ext2 reiserfs; do
	SUFFIX="-postmark.img.oprof"
	tooltype=scan
	fname=${FSL_BASE}/tests/scantool-$fs/$fs$SUFFIX
	printstats

	SUFFIX="-relocate.img.oprof"
	tooltype=relocate
	fname=${FSL_BASE}/tests/relocate-$fs/$fs$SUFFIX
	printstats


	SUFFIX="-scatter.img.oprof"
	tooltype=scatter
	fname=${FSL_BASE}/tests/scattertool-$fs/$fs$SUFFIX
	printstats

	SUFFIX="-defrag.img.oprof"
	tooltype="defrag"
	fname=${FSL_BASE}/tests/defragtool-$fs/$fs$SUFFIX
	printstats

	SUFFIX="-smush.img.oprof"
	tooltype="compact"
	fname=${FSL_BASE}/tests/smushtool-$fs/$fs$SUFFIX
	printstats
	echo "-"
done
