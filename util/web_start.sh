#!/bin/bash

FILESYSTEMS=ext2 vfat iso9660 reiserfs xfs

for a in $FILESYSTEMS; do
	if [ ! -x ./fusebrowse-$a ]; then
		echo "NO FUSEBROWSE-$a"
		exit 1
	fi
	mkdir -p $a
	./fusebrowse-$a $a-postmark.img $a
	ret=$?
	if [ $ret -ne 0 ]; then
		echo "WHOOPS $a couldn't fuse browse"
	fi
done
