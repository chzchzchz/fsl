#!/bin/bash

scanout=tests/scantool-reiserfs/reiserfs.img.out
if [ ! -f $scanout ]; then
	echo "reiserfs: Error no scanout file $scanout "
	exit -1
fi

stats=`grep stat $scanout | sort | uniq | wc -l`
if [ $stats -ne 37 ]; then
	echo "Expected stat items = 37. Got stat items = $stats"
	exit -1
fi

exit 0
