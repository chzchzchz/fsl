#!/bin/bash

source "util/plot/config.sh"

function printstats
{
	if [ ! -e $fname ]; then
		return
	fi
	get_display

	displayname=${displayname}-$tooltype
	raw_bytes=`grep bytes $fname | cut -f2 -d' '`
	echo $displayname $raw_bytes
}

function header
{
	echo Filesystem "Stack Bytes"
}

header
for fs in $FSNAMES; do
	SUFFIX="-postmark.img.stackstat"
	tooltype=scan
	fname=${FSL_BASE}/tests/scantool-$fs/$fs$SUFFIX
	printstats

	SUFFIX="-relocate.img.stackstat"
	tooltype=relocate
	fname=${FSL_BASE}/tests/relocate-$fs/$fs$SUFFIX
	printstats


	SUFFIX="-scatter.img.stackstat"
	tooltype=scatter
	fname=${FSL_BASE}/tests/scattertool-$fs/$fs$SUFFIX
	printstats

	SUFFIX="-defrag.img.stackstat"
	tooltype="defrag"
	fname=${FSL_BASE}/tests/defragtool-$fs/$fs$SUFFIX
	printstats

	SUFFIX="-smush.img.stackstat"
	tooltype="compact"
	fname=${FSL_BASE}/tests/smushtool-$fs/$fs$SUFFIX
	printstats
	echo "-"
done
