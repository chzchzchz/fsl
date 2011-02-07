#!/bin/bash

# as if run from src_root

start_time=`date +%s`
util/draw.ext2.sh
util/draw.vfat.sh
util/draw.nilfs2.sh
util/draw.iso9660.sh
util/draw.reiserfs.sh
util/draw.minix.sh
end_time=`date +%s`

echo "Draw Scans: Total seconds: " `expr $end_time - $start_time`
