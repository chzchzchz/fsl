#!/bin/bash

source "util/plot/config.sh"

#get lines
echo FSL line counts...
for fs in $FSNAMES; do
	echo $fs `wc -l ${FSL_BASE}/fs/*$fs*.fsl | tail -n1 | awk '{print $1;}'`
done >${FSL_BASE}/plots/fsl-lines.dat


echo FuncBytes...
for fs in $FSNAMES; do
        echo $fs `readelf -s ${FSL_BASE}/obj/fs/fsl.$fs.types.o | grep FUNC | awk 'BEGIN { s = 0; } { s += $3;} END {print s;}'`
done >${FSL_BASE}/plots/fsl-funcbytes.dat

echo TablesBytes...
for fs in $FSNAMES; do
	echo $fs `readelf -s ${FSL_BASE}/obj/fs/fsl.$fs.table.o | grep OBJ | awk 'BEGIN { s = 0; } { s += $3;} END {print s;}'`
done>${FSL_BASE}/plots/fsl-tablebytes.dat

echo "Tool SLOCs..."
toolname="defrag browser scan browser relocate scatter smush"
for tool in $toolname; do
	loc=`sloccount ${FSL_BASE}/src/tool/${tool}*  | grep -A1 SLOC |  head -n2 | tail -n1 | cut -f1 -d' '`
	echo $tool $loc
done >fsltool-sloc.dat

# stack depth
echo "Scan depths..."
for fs in $FSNAMES; do
	get_display
	echo "#$fs" >${FSL_BASE}/plots/stackdepth.$fs.dat
	fname="${FSL_BASE}/tests/scantool-$fs/$fs-depth-1.img.stackstat"
	first=`grep bytes $fname | cut -f2 -d' '`
	for dnum in 1 4 16 64 128 192 256; do
		fname="${FSL_BASE}/tests/scantool-$fs/$fs-depth-${dnum}.img.stackstat"
		raw_bytes=`grep bytes $fname | cut -f2 -d' '`
		#echo $dnum `bc -l <<< "$raw_bytes/$first"`
		echo $dnum $raw_bytes
	done >>${FSL_BASE}/plots/stackdepth.$fs.dat
done

echo "Runtime SLOCs..."
sloccount ${FSL_BASE}/src/runtime/* | grep -A1 SLOC  | head -n2 | tail -n1 | cut -f1 -d' '	\
	>${FSL_BASE}/plots/runtime-lines.dat

echo "Compiler SLOCs..."
rm -f ${FSL_BASE}/src/compiler/parser.cc
rm -f ${FSL_BASE}/src/compiler/lex.yy.cc
sloccount ${FSL_BASE}/src/compiler/*.{cc,h,y} | grep -A1 SLOC  | head -n2 | tail -n1 | cut -f1 -d' '	\
	>${FSL_BASE}/plots/compiler-lines.dat

echo "Stat Breakdowns.."
util/plot/get_stack_breakdown.sh >${FSL_BASE}/plots/stack.dat

# get iocache misses from .stats file
function grab_stat
{
	statname="$1"
	statfname="$2"
        grep "$statname" "$statfname" | cut -f4 -d' '
}

function tool_stat
{
	# Get cache misses
	scanner="scanner"
	relocate="relocate"
	scatter="scatter"
	defragment="defrag"
	statname="$1"
	outfile="$2"
	for fs in $FSNAMES; do
		scanner="$scanner "`grab_stat $statname ${FSL_BASE}/tests/scantool-$fs/$fs-postmark.img.stats`
		relocate="$relocate "`grab_stat $statname ${FSL_BASE}/tests/relocate-$fs/$fs-relocate.img.stats`
		scatter="$scatter "`grab_stat $statname ${FSL_BASE}/tests/scattertool-$fs/$fs-scatter.img.stats`
		defragment="$defragment "`grab_stat $statname ${FSL_BASE}/tests/defragtool-$fs/$fs-defrag.img.stats`
	done
	echo "# MISSES FOR DEFAULT TOOL RUNS" >"$outfile"
	echo "Filesystem $FSNAMES" >>"$outfile"
	echo $scanner >>"$outfile"
	echo $relocate >>"$outfile"
	echo $scatter >>"$outfile"

}

echo "Tool cache misses..."
tool_stat "iocache_miss" "${FSL_BASE}/plots/misses.dat"
echo "Tool cache hits..."
tool_stat "iocache_hit" "${FSL_BASE}/plots/hits.dat"


# MAKE FIGURES FOR PAPER
echo "Make the figs..."
gnuplot <util/plot/gnuplot/stack.gnu
gnuplot <util/plot/gnuplot/stackdepth.gnu

echo OK
