#!/bin/bash

export FSL_BASE=`pwd`
export FSNAMES="ext2 vfat reiserfs nilfs2 iso9660"

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
