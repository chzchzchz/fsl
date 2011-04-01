#!/bin/bash

fs="ext2"
source ${src_root}/tests/test_common.sh
source ${src_root}/tests/fs_common.sh
source ${src_root}/tests/fsck.sh

imgname=$fs-depth.img
fsck_img $imgname
fs_scan_startup_img $fs $imgname

imgname=$fs-many.img
fsck_img $imgname
fs_scan_startup_img $fs $imgname

imgname=$fs-postmark.img
fsck_img $imgname
fs_scan_startup_img $fs $imgname

imgname=$fs-relocate.img
cp ${src_root}/img/$fs-many.img ${src_root}/img/$imgname
fs_reloc_startup_img $fs $imgname
fsck_img $imgname
fs_scan_startup_img $fs $imgname

imgname=$fs-scatter.img
cp ${src_root}/img/$fs-postmark.img ${src_root}/img/$imgname
fs_scatter_startup_img $fs $imgname
fsck_img $imgname
fs_scan_startup_img $fs $imgname

imgname=$fs-thrashcopy.img
cp ${src_root}/img/$fs-scatter.img ${src_root}/img/$imgname
fs_thrashcopy_startup_img $fs $imgname
fsck_img $imgname
fs_scan_startup_img $fs $imgname

imgname=$fs-defrag.img
cp ${src_root}/img/$fs-scatter.img ${src_root}/img/$imgname
fs_defrag_startup_img $fs $imgname
fsck_img $imgname
fs_scan_startup_img $fs $imgname

imgname=$fs-smush.img
cp ${src_root}/img/$fs-defrag.img ${src_root}/img/$imgname
fs_smush_startup_img $fs $imgname
fsck_img $imgname
fs_scan_startup_img $fs $imgname

#fsck tests
echo $fs-FSCK Tests

function do_fsck_test
{
	cmd="$1"
	teststr="$2"
	imgname=$fs-fsck.img
	cmdimgname=$fs-"$teststr".img
	cp ${src_root}/img/$fs-postmark.img ${src_root}/img/$imgname
	mv ${src_root}/img/$imgname ${src_root}/img/$cmdimgname
	fs_fuse_cmd_img ext2 "$cmdimgname" "$cmd"
	fsck_img_fail "$cmdimgname"
	fs_fsck_startup_img $fs $cmdimgname
	outfile="${src_root}/tests/fsck-$fs/${cmdimgname}.out"
	grep_str=`grep "$teststr" "$outfile"`
	if [ -z "$grep_str" ]; then
		echo "Did not find expected string $teststr in $cmdimgname"
		echo "Output contents:"
		cat "$outfile"
		exit 1
	fi

	imgname=$fs-fsck.img
	mv ${src_root}/img/$cmdimgname  ${src_root}/img/$imgname
}

do_fsck_test "printf '\xff\xff\xff\xff' >sb/s_first_data_block" "chk_first_dblk"
do_fsck_test "printf '\xff\xff\xff\xff' >sb/s_inodes_c" "chk_inode_count"
