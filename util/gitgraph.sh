#!/bin/bash

OLD_IFS=${IFS}
IFS=$'\n'
dates=( $(git log --format="%ci" --no-merges | tac ) )
IFS=$'\n'
changes=( $(git log --stat --no-merges | grep "files changed" | tac) )
IFS=${OLD_IFS}

numents=${#dates[@]}
echo "#date inserts deletes" >plots/git.dat
inserts_tot=0
deletes_tot=0
for n in `seq 0 $numents`; do
	inserts=`echo "${changes[$n]}" | cut -f5 -d' ' `
	deletes=`echo "${changes[$n]}" | cut -f7 -d' ' `
	printdate=`echo "${dates[$n]}" | cut -f1 -d' ' `

	if [ -z "$inserts" ]; then continue; fi
	if [ -z "$deletes" ]; then continue; fi
	# ignore bogus inserts/deletes
	if [ "$inserts" -gt "10000" ]; then continue; fi
	if [ "$deletes" -gt "10000" ]; then continue; fi

	inserts_tot=`bc <<< "$inserts + $inserts_tot"`
	deletes_tot=`bc <<< "$deletes + $deletes_tot"`
	echo $printdate $inserts_tot $deletes_tot
done >>plots/git.dat

gnuplot <<< "
set xdata time
set timefmt \"%Y-%m-%d\"
set ylabel 'Lines'
set xlabel 'Date'
set key left top
set title 'Changed Lines over Time'
set xtic rotate by -45 scale 0
set term png font \"/usr/share/fonts/ttf-bitstream-vera/Vera.ttf\"
set output \"plots/git.png\"
plot 'plots/git.dat' using 1:2 with linespoints title 'Inserts', '' using 1:3 with linespoints title 'Deletes'
"