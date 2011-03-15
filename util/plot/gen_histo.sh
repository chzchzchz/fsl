#!/bin/bash

function set_gnuscript
{
	TITLE="$1"
	OUTFILE="$2"
	YLABEL="$3"
	XLABEL="$4"
	SRCFILE="$5"
gnuscript_dat="
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
"

plotcmd="plot '$SRCFILE' using 2:xtic(1) ti col"
total=`expr $6 - 1`
for x in `seq $total`; do
	echo ">>>>>>>$x"
	v=`expr $x + 2`
	plotcmd="$plotcmd , ""'' u $v ti col"
done
plotcmd="$plotcmd
"
gnuscript_dat="$gnuscript_dat $plotcmd"
}

function plot_dat
{
	set_gnuscript "$1" "$2" "$3" "$4" "$5" "$6"
	echo $gnuscript_dat
	gnuplot <<<"$gnuscript_dat"
}

plot_dat "TITLE" "SOMEFILE" "YLABEL" "XLABEL" "TEST.DAT" 3
