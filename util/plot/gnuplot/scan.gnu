#set terminal postscript eps size 5,5 enhanced     font 'Helvetica,20' linewidth 2
set terminal postscript eps size 4,4 enhanced     font 'Helvetica,20' linewidth 2
set datafile missing '-'

set auto x
set yrange [0:]
set style data histogram
set style histogram cluster gap 1
set style fill pattern border -1
set boxwidth 1
#set xtic rotate by -45 scale 0
set ylabel 'Time (Seconds)' offset 1.5,0
set bmargin 10

set title 'Scanning Real Execution Time'
set output 'plots/scan-real.eps'
plot 'plots/scan-real.dat' using 2:xtic(1) ti col, '' u 3 ti col, '' u 4 ti col

set title 'Scanning Sys Execution Time'
set output 'plots/scan-sys.eps'
plot 'plots/scan-sys.dat' using 2:xtic(1) ti col, '' u 3 ti col, '' u 4 ti col

set title 'Scanning User Execution Time'
set output 'plots/scan-user.eps'
plot 'plots/scan-user.dat' using 2:xtic(1) ti col, '' u 3 ti col, '' u 4 ti col


set terminal png font "/usr/share/fonts/ttf-bitstream-vera/Vera.ttf"
set title 'Scanning Real Execution Time'
set output 'plots/scan-real.png'
plot 'plots/scan-real.dat' using 2:xtic(1) ti col, '' u 3 ti col, '' u 4 ti col

set title 'Scanning Sys Execution Time'
set output 'plots/scan-sys.png'
plot 'plots/scan-sys.dat' using 2:xtic(1) ti col, '' u 3 ti col, '' u 4 ti col

set title 'Scanning User Execution Time'
set output 'plots/scan-user.png'
plot 'plots/scan-user.dat' using 2:xtic(1) ti col, '' u 3 ti col, '' u 4 ti col
