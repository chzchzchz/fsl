#set terminal postscript eps size 5,5 enhanced     font 'Helvetica,20' linewidth 2
set terminal postscript eps size 4,4 enhanced     font 'Helvetica,20' linewidth 2
set output 'plots/frag.eps'
set datafile missing '-'

set title 'Fragmented tar execution time'
set auto x
set yrange [1:50000]
set style data histogram
set style histogram cluster gap 1
set style fill pattern border -1
set boxwidth 0.9
#set xtic rotate by -45 scale 0
set logscale y
set ylabel 'Time (Seconds)' offset 1.5,0
set bmargin 10
set key top center
plot 'plots/frag.dat' using 2:xtic(1) ti col, '' u 3 ti col, '' u 4 ti col, '' u 5 ti col
