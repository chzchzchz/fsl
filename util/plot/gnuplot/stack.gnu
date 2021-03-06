#set terminal postscript eps size 5,5 enhanced     font 'Helvetica,20' linewidth 2
set terminal postscript eps size 4,4 enhanced     font 'Helvetica,20' linewidth 2
set output 'plots/stack.eps'
set datafile missing '-'

set title 'Stack Utilization'
set auto x
set yrange [0:]
set style data histogram
set style histogram cluster gap 0
set style fill pattern border -1
set boxwidth 0.9
set xtic rotate by -45 scale 0
set key off
set ylabel 'Maximum Stack Use (bytes)' offset 1.5,0
set bmargin 13
plot 'plots/stack.dat' using 2:xtic(1) fs pattern 2

set terminal png font "/usr/share/fonts/ttf-bitstream-vera/Vera.ttf"
set output 'plots/stack.png'
replot
