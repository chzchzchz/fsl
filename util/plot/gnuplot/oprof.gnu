#set terminal postscript eps size 5,5 enhanced     font 'Helvetica,20' linewidth 2
set terminal postscript eps size 4,4 enhanced     font 'Helvetica,20' linewidth 2
set output 'plots/oprof.eps'
set datafile missing '-'

set title 'Percent Execution Time Breakdown'
set auto x
set yrange [0:100]
set style data histogram
set style histogram rowstacked
set style fill pattern border -1
set boxwidth 1
set ylabel '% Execution' offset 2.5,0
set auto x
set bmargin 13
set key autotitle columnheader
#set key reverse Left outside width -5 samplen 2
set key reverse Left rmargin samplen 1 width -2
set xtic rotate by -45 scale 0
plot 'plots/oprof.dat' using 2:xtic(1) ti col, '' u 3 ti col, '' u 4 ti col, '' u 5 ti col, '' u 6 ti col

set terminal png font "/usr/share/fonts/ttf-bitstream-vera/Vera.ttf"
set output 'plots/oprof.png'
replot
