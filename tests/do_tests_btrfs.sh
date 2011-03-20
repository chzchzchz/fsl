#!/bin/bash

src_root=`pwd`
TESTBASE="tests/fusebrowse-btrfs"
source `pwd`/tests/test_common.sh
source `pwd`/tests/fs_common.sh

echo Testing fusebrowse-btrfs
fs_fuse_cmd_img btrfs btrfs-empty.img "ls -la sb/root_tree/0/leaf/itemdat/3" "inoref"
fs_fuse_cmd_img btrfs btrfs-empty.img "ls -la sb/root_tree/0/leaf/itemdat/2" "diritem"
fs_fuse_cmd_img btrfs btrfs-empty.img "cat sb/root_tree/0/leaf/itemdat/2/dir_item/name" "dirname"
fs_fuse_cmd_img btrfs btrfs.img "od -td sb/root_tree/0/leaf/itemdat/5/root_item/root_data/hdr/level" "rootlevel1"

c_inoref=`cat $TESTBASE/btrfs-empty.img-inoref.out | awk '{ print $5 " " $9; }'`
c_diritem=`cat $TESTBASE/btrfs-empty.img-diritem.out | awk '{ print $5 " " $9; }'`
c_dirname=`cat $TESTBASE/btrfs-empty.img-dirname.out | grep "default"`
c_rootl1=`cat $TESTBASE/btrfs.img-rootlevel1.out | awk '{ print $2 }' | grep "1"`

inoref_str=`echo "$c_inoref" | grep "ino_ref"`
if [ -z "$inoref_str" ]; then
	echo "INO REF NOT FOUND"
	cat $TESTBASE/btrfs-empty.img-inoref.out
	exit 1
fi

diritem_str=`echo "$c_diritem" | grep "dir_item"`
if [ -z "$diritem_str" ]; then
	echo "DIRITEM NOT FOUND"
	cat $TESTBASE/btrfs-empty.img-diritem.out
	exit 1
fi

if [ -z "$c_dirname" ]; then
	echo "DIRNAME NOT FOUND"
	cat $TESTBASE/btrfs-empty.img-dirname.out
	exit 1
fi

if [ -z "$c_rootl1" ]; then
	echo "ROOTL1 NOT FOUND"
	cat $TESTBASE/btrfs.img-rootlevel1.out
	exit 1
fi
