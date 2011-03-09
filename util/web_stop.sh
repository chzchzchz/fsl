#!/bin/bash
FILESYSTEMS=ext2 vfat iso9660 reiserfs xfs
for a in $FILESYSTEMS; do
	fusermount -u $a
done
