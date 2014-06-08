set title 'FSCK Reads XFS'
set ylabel 'Disk Offset'
set terminal postscript eps size 4,4 enhanced     font 'Helvetica,20' linewidth 2
set output 'plots/xfs-fsck-io.eps'
plot 'plots/xfs-fsck.read.out'
set terminal png font "/usr/share/fonts/ttf-bitstream-vera/Vera.ttf"
set output 'plots/xfs-fsck-io.png'
replot

set title 'FSCK Reads MINIX'
set terminal postscript eps size 4,4 enhanced     font 'Helvetica,20' linewidth 2
set output 'plots/minix-fsck-io.eps'
plot 'plots/minix-fsck.read.out'
set terminal png font "/usr/share/fonts/ttf-bitstream-vera/Vera.ttf"
set output 'plots/minix-fsck-io.png'
replot

set title 'FSCK Reads VFAT'
set terminal postscript eps size 4,4 enhanced     font 'Helvetica,20' linewidth 2
set output 'plots/vfat-fsck-io.eps'
plot 'plots/vfat-fsck.read.out'
set terminal png font "/usr/share/fonts/ttf-bitstream-vera/Vera.ttf"
set output 'plots/vfat-fsck-io.png'
replot

set title 'FSCK Reads EXT2'
set terminal postscript eps size 4,4 enhanced     font 'Helvetica,20' linewidth 2
set output 'plots/ext2-fsck-io.eps'
plot 'plots/ext2-fsck.read.out'
set terminal png font "/usr/share/fonts/ttf-bitstream-vera/Vera.ttf"
set output 'plots/ext2-fsck-io.png'
replot

set title 'FSCK Reads ReiserFS'
set terminal postscript eps size 4,4 enhanced     font 'Helvetica,20' linewidth 2
set output 'plots/reiserfs-fsck-io.eps'
set xrange [5:2070]
plot 'plots/reiserfs-fsck.read.out'
set terminal png font "/usr/share/fonts/ttf-bitstream-vera/Vera.ttf"
set xrange [5:2070]
set output 'plots/reiserfs-fsck-io.png'
replot

reset
set xrange [4:]
set title 'FSCK Reads BTRFS'
set terminal postscript eps size 4,4 enhanced     font 'Helvetica,20' linewidth 2
set output 'plots/btrfs-fsck-io.eps'
plot 'plots/btrfs-fsck.read.out'
set terminal png font "/usr/share/fonts/ttf-bitstream-vera/Vera.ttf"
set output 'plots/btrfs-fsck-io.png'
replot
