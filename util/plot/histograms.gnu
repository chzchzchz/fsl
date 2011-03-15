set terminal png transparent nocrop enhanced small
set output '$OUTFILE.png'
set datafile missing '-'


set title '$TITLE'
set auto x
set yrange [0:10]
set style data histogram
set style histogram cluster gap 1
set style fill solid border -1
set boxwidth 0.9
set xtic rotate by -45 scale 0
set ylabel '$YLABEL'
set xlabel '$XLABEL'
set bmargin 10
plot 'TEST.DAT' using 2:xtic(1) ti col, '' u 3 ti col, '' u 4 ti col
