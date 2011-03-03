#!/bin/sh

echo "===========EXT2 TESTS============="

ls -lah /mnt/fslfs 2>&1 >ext2_ls_lah.test
ls -lah /mnt/fslfs/root_ino 2>&1 >ext2_rootino.test
ls -lah /mnt/fslfs/grp_desc_table 2>&1 >ext2_grpdesc.test
ls -lah /mnt/fslfs/root_ino/0 2>&1 >ext2_rootino_dat.test
echo FFFFFFFFFF
echo asddddddddd
#od -Ax -tx /mnt/fslfs/grp_desc_table/3/block_bmp 2>&1 >ext2_od_grp3_blkbmp.test
