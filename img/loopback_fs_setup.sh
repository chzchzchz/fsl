#!/bin/bash

# associate image with lo, mount fs image

FS_IMAGE="$1"
FS_TYPE="$2"

if [ -z $FS_IMAGE ]; then
	echo "No file given!"
	exit -1
fi

if [ -z $FS_DIR ]; then
	FS_DIR=/mnt/FSTEST/
fi


sudo /sbin/losetup /dev/loop5 "$FS_IMAGE"

retval=$?
if [ $retval -ne 0 ]; then
	exit $retval
fi

FLAGS=""
if [ "$FS_TYPE" = "vfat" ]; then
	FLAGS=-oumask=000,uid=1000
	echo $FLAGS
fi

sudo /bin/mount -t "$FS_TYPE" /dev/loop5 "$FS_DIR" $FLAGS
retval=$?

if [ $retval -ne 0 ]; then
	echo "Bad mount!"
	sudo /sbin/losetup -d /dev/loop5
	exit $retval
fi

exit 0
