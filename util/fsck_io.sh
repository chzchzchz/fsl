#!/bin/bash

#remember to run from git root!

if [ ! -d plots ]; then
	echo "Plots directory not found. Are you running from git root?"
fi

echo EXT2 FSCK
IO_INST_READFILE=plots/ext2-fsck.read.out	\
IO_INST_WRITEFILE=plots/ext2-fsck.write.out 	\
IO_INST_TRACKFD=5				\
LD_PRELOAD=`pwd`/lib/io_inst/io_inst.so		\
/sbin/fsck.ext2 -f img/ext2-postmark.img

echo VFAT FSCK
IO_INST_READFILE=plots/vfat-fsck.read.out	\
IO_INST_WRITEFILE=plots/vfat-fsck.write.out 	\
IO_INST_TRACKFD=5				\
LD_PRELOAD=`pwd`/lib/io_inst/io_inst.so		\
/sbin/fsck.vfat -f img/vfat-postmark.img

echo MINIX FSCK
IO_INST_READFILE=plots/minix-fsck.read.out	\
IO_INST_WRITEFILE=plots/minix-fsck.write.out 	\
IO_INST_TRACKFD=5				\
LD_PRELOAD=`pwd`/lib/io_inst/io_inst.so		\
/sbin/fsck.minix -f img/minix-many.img

echo XFS FSCK
IO_INST_READFILE=plots/xfs-fsck.read.out	\
IO_INST_WRITEFILE=plots/xfs-fsck.write.out 	\
IO_INST_TRACKFD=5				\
LD_PRELOAD=`pwd`/lib/io_inst/io_inst.so		\
/usr/sbin/xfs_db -f -c "check" -f img/xfs-postmark.img

echo REISER FSCK
IO_INST_READFILE=plots/reiserfs-fsck.read.out		\
IO_INST_WRITEFILE=plots/reiserfs-fsck.write.out 	\
IO_INST_TRACKFD=5				\
LD_PRELOAD=`pwd`/lib/io_inst/io_inst.so		\
/sbin/reiserfsck -f -y img/reiserfs-postmark.img

echo BTRFS FSCK
IO_INST_READFILE=plots/btrfs-fsck.read.out	\
IO_INST_WRITEFILE=plots/btrfs-fsck.write.out 	\
IO_INST_TRACKFD=6				\
LD_PRELOAD=`pwd`/lib/io_inst/io_inst.so		\
/sbin/btrfsck img/btrfs-postmark.img