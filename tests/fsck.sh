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
	case $fs in
	"ext2")		FSCKLODEV=/dev/loop1	;;
	"reiserfs")	FSCKLODEV=/dev/loop2	;;
	"minix")	FSCKLODEV=/dev/loop3	;;
	"xfs")		FSCKLODEV=/dev/loop4	;;
	"vfat")		FSCKLODEV=/dev/loop6	;;
	"btrfs")	FSCKLODEV=/dev/loop8	;;
	*)		FSCKLODEV=/dev/loop9	;;
	esac
}

function fsck_get_cmd
{
	case $fs in
	"ext2")		FSCKCMD="sudo /sbin/fsck.ext2 -v -f -n " ;;
	"reiserfs")	FSCKCMD="sudo /sbin/fsck.reiserfs -f -y " ;;
	"minix")	FSCKCMD="sudo /sbin/fsck.minix -v -s -f " ;;
	"btrfs")	FSCKCMD="sudo /sbin/btrfsck " ;;
	"xfs")		FSCKCMD="sudo /sbin/xfs_check " ;;
	*)		FSCKCMD="sudo fsck " ;;
	esac
}

function fsck_img_setup
{
	if [ -z $1 ]; then
		echo "NO IMAGENAME GIVEN"
		exit -2
	fi

	fsck_get_lodev
	fsck_get_cmd
}

function fsck_img
{
	fsck_img_setup "$1"
	fsck_imgname="$1"
	echo "FSCKING ${fsck_imgname}"

	sudo /sbin/losetup $FSCKLODEV ${src_root}/img/${fsck_imgname}
	$FSCKCMD $FSCKLODEV 				\
		2>${src_root}/fsck.fail.$fs.stderr	\
		>${src_root}/fsck.fail.$fs.stdout
	retval=$?
	sudo /sbin/losetup -d $FSCKLODEV
	if [ $retval -ne 0 ]; then
		echo "FSCK FAILED ON ${fsck_imgname}. EXITCODE=$retval"
		exit $retval
	fi
}

# expect fsck to fail
function fsck_img_fail
{
	fsck_img_setup "$1"
	fsck_imgname="$1"
	echo "FSCKING ${fsck_imgname}"
	sudo /sbin/losetup $FSCKLODEV ${src_root}/img/${fsck_imgname}
	$FSCKCMD $FSCKLODEV 				\
		2>${src_root}/fsck.fail.$fs.stderr	\
		>${src_root}/fsck.fail.$fs.stdout
	retval=$?
	sudo /sbin/losetup -d $FSCKLODEV
	if [ $retval -eq 0 ]; then
		echo "FSCK OK ON ${fsck_imgname}. But should fail!"
		exit $retval
	fi
}
