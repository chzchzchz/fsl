#!/bin/bash

export FSL_BASE=`pwd`
export FSNAMES="iso9660 vfat ext2 reiserfs nilfs2 xfs minix"

function get_display
{
	if [ $fs = 'vfat' ]; then
		displayname="VFAT"
	elif [ $fs = 'reiserfs' ]; then
		displayname='ReiserFS'
	elif [ $fs = 'xfs' ]; then
		displayname="XFS"
	elif [ $fs = 'minix' ]; then
		diplsyname="MinixFS"
	else
		displayname=$fs
	fi
}
