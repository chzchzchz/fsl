#set terminal postscript eps size 5,5 enhanced     font 'Helvetica,20' linewidth 2
set terminal postscript eps size 4,4 enhanced     font 'Helvetica,20' linewidth 2
set output 'plots/hits.eps'
set datafile missing '-'

set title 'Cache Hits By Tool'
set auto x
set style data histogram
set style histogram cluster gap 1
set style fill pattern border -1
set boxwidth 0.9
#set xtic rotate by -45 scale 0
set logscale y
set ylabel 'Cache Hits' offset 1.5,0
set bmargin 10
#set key top center
set key outside
plot 'plots/hits.dat' using 2:xtic(1) ti col, '' u 3 ti col, '' u 4 ti col, '' u 5 ti col

set terminal png font "/usr/share/fonts/ttf-bitstream-vera/Vera.ttf"
set output 'plots/hits.png'
replot
