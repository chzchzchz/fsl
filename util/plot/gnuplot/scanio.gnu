set title 'FSL Reads XFS'
set xtic rotate by -45 scale 0
set ylabel 'Disk Offset'
set terminal postscript eps size 4,4 enhanced     font 'Helvetica,20' linewidth 2
set output 'plots/xfs-fsl-io.eps'
plot 'tests/scantool-xfs/xfs-postmark.img.misslog' using ($1/8) title 'xfs-postmark'
set terminal png font "/usr/share/fonts/ttf-bitstream-vera/Vera.ttf"
set output 'plots/xfs-fsl-io.png'
replot

set title 'FSL Reads MINIX'
set terminal postscript eps size 4,4 enhanced     font 'Helvetica,20' linewidth 2
set output 'plots/minix-fsl-io.eps'
plot 'tests/scantool-minix/minix-many.img.misslog' using ($1/8) title 'minix-many'
set terminal png font "/usr/share/fonts/ttf-bitstream-vera/Vera.ttf"
set output 'plots/minix-fsl-io.png'
replot

set title 'FSL Reads VFAT'
set terminal postscript eps size 4,4 enhanced     font 'Helvetica,20' linewidth 2
set output 'plots/vfat-fsl-io.eps'
plot 'tests/scantool-vfat/vfat-postmark.img.misslog' using ($1/8) title 'vfat-postmark'
set terminal png font "/usr/share/fonts/ttf-bitstream-vera/Vera.ttf"
set output 'plots/vfat-fsl-io.png'
replot

set title 'FSL Reads EXT2'
set terminal postscript eps size 4,4 enhanced     font 'Helvetica,20' linewidth 2
set output 'plots/ext2-fsl-io.eps'
plot 'tests/scantool-ext2/ext2-postmark.img.misslog' using ($1/8) title 'ext2-postmark'
set terminal png font "/usr/share/fonts/ttf-bitstream-vera/Vera.ttf"
set output 'plots/ext2-fsl-io.png'
replot

set title 'FSL Reads ReiserFS'
set terminal postscript eps size 4,4 enhanced     font 'Helvetica,20' linewidth 2
set output 'plots/reiserfs-fsl-io.eps'
plot 'tests/scantool-reiserfs/reiserfs-postmark.img.misslog' using ($1/8) title 'reiserfs-postmark'
set terminal png font "/usr/share/fonts/ttf-bitstream-vera/Vera.ttf"
set output 'plots/reiserfs-fsl-io.png'
replot

set title 'FSL Reads BTRFS'
set terminal postscript eps size 4,4 enhanced     font 'Helvetica,20' linewidth 2
set output 'plots/btrfs-fsl-io.eps'
plot 'tests/scantool-btrfs/btrfs-postmark.img.misslog' using ($1/8) title 'btrfs-postmark'
set terminal png font "/usr/share/fonts/ttf-bitstream-vera/Vera.ttf"
set output 'plots/btrfs-fsl-io.png'
replot