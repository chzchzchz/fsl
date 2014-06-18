#!/bin/bash

if [ -z ${src_root} ]; then
	echo "NO SOURCE ROOT DECLARED"
	exit -1
fi

if [ -z $fs ]; then
	echo "NO FS DECLARED"
fi

function do_fsck_test
{
	cmd="$1"
	teststr="$2"
	imgname=$fs-fsck.img
	srcimgname=$fs-postmark.img
	if [ ! -z "$3" ]; then
		srcimgname="$3"
	fi

	cmdimgname=$fs-"$teststr".img
	cp ${src_root}/img/$srcimgname ${src_root}/img/$imgname
	mv ${src_root}/img/$imgname ${src_root}/img/$cmdimgname
	fs_fuse_cmd_img $fs "$cmdimgname" "$cmd"
	fsck_img_fail "$cmdimgname"
	fs_fsck_startup_img_failable $fs $cmdimgname
	outfile="${src_root}/tests/fsck-$fs/${cmdimgname}.out"
	grep_str=`grep "$teststr" "$outfile"`
	if [ -z "$grep_str" ]; then
		echo "Did not find expected string $teststr in $cmdimgname"
		echo "Output contents ($outfile):"
		cat "$outfile"
		exit 1
	fi

	imgname=$fs-fsck.img
	mv ${src_root}/img/$cmdimgname  ${src_root}/img/$imgname
}

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
	"vfat")		FSCKCMD="sudo /usr/sbin/dosfsck -n " ;;
	"nilfs2")	FSCKCMD="true " ;; # no fsck for nilfs2!
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

	outpath=${src_root}/
	if [ ! -z "$fs" ]; then
		outpath=${src_root}/tests/fsck-$fs/
	fi

	sudo /sbin/losetup $FSCKLODEV ${src_root}/img/${fsck_imgname}
	$FSCKCMD $FSCKLODEV 				\
		2>$outpath/fsck.fail.${fsck_imgname}.stderr	\
		>$outpath/fsck.fail.${fsck_imgname}.stdout
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
