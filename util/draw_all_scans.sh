#!/bin/bash

# as if run from src_root

function draw_ext2
{
	if [ ! -f ext2.scan.out ]; then
		echo "No files generated? Run some tests first."
		exit -1
	fi

	grep Mode ext2.scan.out | egrep "(dir|sb|group_desc|bmp|ext2_inode'|ext2_data_block)" >ext2.scan.processed.out
	ext2_sz=`ls -l img/ext2.img  | awk '{ print $5; }'`

	./util/draw_scan.py ext2.scan.processed.out $ext2_sz ext2.png >ext2.draw.log
}


function draw_vfat
{
	if [ ! -f vfat.scan.out ]; then
		echo "No files generated? Run some tests first."
		exit -1
	fi

	grep Mode vfat.scan.out | egrep "(fat'|fat_de|fat_ent|bpb|'voff' : 0.*cluster)" >vfat.scan.processed.out
	vfat_sz=`ls -l img/vfat.img  | awk '{ print $5; }'`

	./util/draw_scan.py vfat.scan.processed.out $vfat_sz vfat.png >vfat.draw.log

}

draw_ext2
draw_vfat
