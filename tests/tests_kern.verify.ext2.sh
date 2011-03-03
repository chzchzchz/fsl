#!/bin/bash

TESTDIR=tests/kernel-ext2/
SZ_AND_NAME="awk ' { print \$5 \" \" \$8; }'"

chk_desc12=`cat $TESTDIR/ext2_grpdesc.test | eval "$SZ_AND_NAME" | grep '32 12'`
lslah_fields=`cat $TESTDIR/ext2_ls_lah.test  | eval "$SZ_AND_NAME" | egrep '(blocks|grp_desc_table|root_ino|sb)'`
chk_rootino=`cat $TESTDIR/ext2_rootino.test  | eval "$SZ_AND_NAME" | grep '128 0' `
chk_lslah_fields=`wc -l <<<"$lslah_fields" | grep ' 4 '`
chk_blocks=`cat $TESTDIR/ext2_ls_lah.test  | eval "$SZ_AND_NAME" | grep "100M blocks" `
chk_grptab=`cat $TESTDIR/ext2_ls_lah.test  | eval "$SZ_AND_NAME" | grep "416 grp_desc_table" `
chk_rootvdir=`cat $TESTDIR/ext2_rootino_dat.test  | eval "$SZ_AND_NAME" | grep '0 vdir' `
chk_od=`grep 00006103 $TESTDIR/ext2_od_grp3_blkbmp.test`

if [ -z "$chk_desc12" ]; then
	echo "CHKDESC12"
	cat $TESTDIR/ext2_grpdesc.test
	exit 1
fi

if [ -z "$chk_rootino" ]; then
	echo "CHKROOTINO"
	cat $TESTDIR/ext2_rootino.test
	exit 2
fi

if [ -z "$chk_desc12" ]; then
	echo "CHKDESC12"
	cat $TESTDIR/ext2_grpdesc.test
	exit 3
fi
if [ -z "$chk_desc12" ]; then
	echo "CHKDESC12"
	cat $TESTDIR/ext2_grpdesc.test
	exit 4
fi

if [ -z "$chk_blocks" ]; then
	echo chk_blocks
	cat $TESTDIR/ext2_ls_lah.test
	exit 5
fi

if [ -z "$chk_grptab" ]; then
	echo chk_grptab
	cat $TESTDIR/ext2_ls_lah.test
	exit 6
fi

if [ -z "$chk_rootvdir" ]; then
	echo chk_rootvdir
	cat $TESTDIR/ext2_rootino_dat.test
	exit 7
fi

if [ -z "$chk_od" ]; then
	echo "Failed to read group block bitmap pointer"
	cat $TESTDIR/ext2_od_grp3_blkbmp.test
	exit 8
fi