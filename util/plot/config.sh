#!/bin/bash

export FSL_BASE=`pwd`
export FSNAMES="iso9660 vfat ext2 reiserfs nilfs2"

function get_display
{
	if [ $fs = 'vfat' ]; then
		displayname="VFAT"
		return
	fi
	if [ $fs = 'reiserfs' ]; then
		displayname='ReiserFS'
		return
	fi
	displayname=$fs
}
