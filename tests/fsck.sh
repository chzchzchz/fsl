#!/bin/bash

if [ -z ${src_root} ]; then
	echo "NO SOURCE ROOT DECLARED"
	exit -1
fi

if [ -z $fs ]; then
	echo "NO FS DECLARED"
fi

function fsck_get_lodev
{
	if [ $fs = "ext2" ]; then
		FSCKLODEV=/dev/loop1
	elif [ $fs = "reiserfs" ]; then
		FSCKLODEV=/dev/loop2
	else
		FSCKLODEV=/dev/loop3
	fi
}

function fsck_get_cmd
{
	case $fs in
	"ext2")		FSCKCMD="sudo /sbin/fsck.ext2 -v -f -n " ;;
	"reiserfs")	FSCKCMD="sudo /sbin/fsck.reiserfs -f -y " ;;
	"minix")	FSCKCMD="sudo /sbin/fsck.minix -v -s -f " ;;
	"btrfs")	FSCKCMD="sudo /sbin/btrfsck " ;;
	*)		FSCKCMD="sudo fsck " ;;
	esac
}

function fsck_img
{
	if [ -z $1 ]; then
		echo "NO IMAGENAME GIVEN"
		exit -2
	fi

	fsck_imgname="$1"
	fsck_get_lodev
	fsck_get_cmd
	echo "FSCKING ${fsck_imgname}"
	sudo /sbin/losetup $FSCKLODEV ${src_root}/img/${fsck_imgname}
	$FSCKCMD $FSCKLODEV 2>${src_root}/fsck.fail.$fs.stderr  >${src_root}/fsck.fail.$fs.stdout
	retval=$?
	sudo /sbin/losetup -d $FSCKLODEV
	if [ $retval -ne 0 ]; then
		echo "FSCK FAILED ON ${fsck_imgname}. EXITCODE=$retval"
		exit $retval
	fi
}
