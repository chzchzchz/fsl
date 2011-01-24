#!/bin/bash

src_root=`pwd`

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

echo Testing fusebrowse-reiserfs
${src_root}/bin/fusebrowse-reiserfs img/reiserfs.img tmp
ls -la tmp  >tests/fusebrowse-reiserfs/ls.out
p=`cat tests/fusebrowse-reiserfs/ls.out | awk '{ print $5 " " $9; }'`
fusermount -u tmp
sb_str=`echo "$p" | grep "204 sb_new"`
if [ -z "$sb_str" ]; then
	echo "BAD SB STR"
	echo "$p"
	exit 1
fi

rawblks_str=`echo "$p" | grep "104857600 raw_blocks"`
if [ -z "$sb_str" ]; then
	echo "BAD RAWBLK BYTES"
	echo "$p"
	exit 1
fi


exit 0
