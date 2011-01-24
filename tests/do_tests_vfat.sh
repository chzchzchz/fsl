#!/bin/bash

tool_name="browser-vfat"
img_name="vfat.img"
source `pwd`/tests/test_common.sh
src_root=`pwd`

run_test 1 "Visit Root DE[1]"
run_test_no_match 2 "Virt-If. Non-File" "vfile"
run_test_match 3 "Virt-If. File." "vfile"

# don't get boot sector!!
run_test_no_match 4 "Virtual. Read file xlate." "55 aa"

echo Testing fusebrowse-vfat
${src_root}/bin/fusebrowse-vfat img/vfat.img tmp
ls -la tmp  >tests/fusebrowse-vfat/ls.out
ret_ls=$?
ls -la tmp/d_bpb  >tests/fusebrowse-vfat/ls_bpb.out
ret_ls2=$?
fusermount -u tmp
if [ $ret_ls -ne 0 ]; then
	echo "BAD FUSE LS /"
	exit $ret_ls
fi

if [ $ret_ls2 -ne 0 ]; then
	echo "BAD FUSELS /BPB"
	exit $ret_ls2
fi

p=`cat tests/fusebrowse-vfat/ls.out | awk '{ print $5 " " $9; }'`
p_bpb=`cat tests/fusebrowse-vfat/ls_bpb.out | awk '{ print $5 " " $9; }'`

sb_str=`echo "$p" | grep "62 d_bpb"`
if [ -z "$sb_str" ]; then
	echo "BAD BPB"
	echo "$p"
	exit 1
fi

fats_str=`echo $p | grep "204800 fats"`
if [ -z "$fats_str" ]; then
	echo "BAD FAT LENGTH"
	echo "$p"
	exit 1
fi


exit 0
