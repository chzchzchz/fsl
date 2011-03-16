set terminal postscript eps size 4,4 enhanced     font 'Helvetica,20' linewidth 2
set output 'plots/hit-miss.eps'
set datafile missing '-'

set title 'Hit-Miss Breakdown'
set auto x
set style data histogram
set style histogram rowstacked
set style fill pattern border -1
set boxwidth 1
set ylabel 'Accesses' offset 2.5,0
set auto x
set bmargin 13
set key autotitle columnheader
#set key reverse Left outside width -5 samplen 2
set key reverse Left rmargin samplen 1 width -2
set xtic rotate by -45 scale 0
plot 'plots/hit-miss.dat' using 2:xtic(1) ti col, '' u 3 ti col
#, '' u 4 ti col, '' u 5 ti col, '' u 6 ti col

set terminal png font "/usr/share/fonts/ttf-bitstream-vera/Vera.ttf"
set output 'plots/hit-miss.png'
replot
