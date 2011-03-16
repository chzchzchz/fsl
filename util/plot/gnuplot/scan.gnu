#set terminal postscript eps size 5,5 enhanced     font 'Helvetica,20' linewidth 2
set terminal postscript eps size 4,4 enhanced     font 'Helvetica,20' linewidth 2
set output 'plots/scan.eps'
set datafile missing '-'

set title 'Scanning Execution Time'
set auto x
set yrange [0:]
set style data histogram
set style histogram cluster gap 1
set style fill pattern border -1
set boxwidth 1
#set xtic rotate by -45 scale 0
set ylabel 'Time (Seconds)' offset 1.5,0
set bmargin 10
plot 'plots/scan.dat' using 2:xtic(1) ti col, '' u 3 ti col

set terminal png font "/usr/share/fonts/ttf-bitstream-vera/Vera.ttf"
set output 'plots/scan.png'
plot 'plots/scan.dat' using 2:xtic(1) ti col, '' u 3 ti col
