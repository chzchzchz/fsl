#!/bin/bash

# Run bonnie on all loopback disks

function run_bonnie
{
	FSIMAGERAW="$1"
	FSIMAGE=img/"$1"
	FSTYPE="$2"
	echo "Bonnie: $FSIMAGE"
	img_sz=`ls -l $FSIMAGE | awk '{ print $5; }'`
	img_mb=`expr $img_sz / 1048576`
	scratch_mb=`expr $img_mb / 3`
	./img/loopback_fs_setup.sh "$FSIMAGE" "$FSTYPE"
	bonnie -d "/mnt/FSTEST/" -s $scratch_mb >bonne.out."$FSIMAGERAW".txt
	./img/loopback_fs_teardown.sh
}

run_bonnie ext2.img ext2
run_bonnie vfat.img vfat
run_bonnie nilfs.img nilfs2
