#set terminal postscript eps size 5,5 enhanced     font 'Helvetica,20' linewidth 2
set terminal postscript eps size 4,3 enhanced     font 'Helvetica,20' linewidth 2
set output 'plots/stackdepth.eps'
set datafile missing '-'

set title 'Scan Stack Utilization By Directory Depth'
set xtics ("1" 1, "16" 16, "64" 64, "128" 128,  "192" 192,"256" 256)
set xlabel "Depth of Directory Hierarchy"
set ylabel 'Maximum Stack Use (bytes)' offset 1.5,0
plot	'plots/stackdepth.vfat.dat' with linespoints title 'VFAT' ps 2, \
	'plots/stackdepth.ext2.dat' with linespoints title 'ext2' ps 2, \
	'plots/stackdepth.reiserfs.dat' with linespoints title 'ReiserFS' ps 2

set terminal png font "/usr/share/fonts/ttf-bitstream-vera/Vera.ttf"
set output 'plots/stackdepth.png'
replot
