#!/bin/bash

source "util/plot/config.sh"

PLOTSDIR="${FSL_BASE}/plots"

#get lines
echo FSL line counts...
for fs in $FSNAMES; do
	echo $fs `wc -l ${FSL_BASE}/fs/*$fs*.fsl | tail -n1 | awk '{print $1;}'`
done >${PLOTSDIR}/fsl-lines.dat


echo FuncBytes...
for fs in $FSNAMES; do
        echo $fs `readelf -s ${FSL_BASE}/obj/fs/fsl.$fs.types.o | grep FUNC | awk 'BEGIN { s = 0; } { s += $3;} END {print s;}'`
done >${PLOTSDIR}/fsl-funcbytes.dat

echo TablesBytes...
for fs in $FSNAMES; do
	echo $fs `readelf -s ${FSL_BASE}/obj/fs/fsl.$fs.table.o | grep OBJ | awk 'BEGIN { s = 0; } { s += $3;} END {print s;}'`
done>${PLOTSDIR}/fsl-tablebytes.dat

echo "Tool SLOCs..."
toolname="defrag browser scan browser relocate scatter smush"
for tool in $toolname; do
	loc=`sloccount ${FSL_BASE}/src/tool/${tool}*  | grep -A1 SLOC |  head -n2 | tail -n1 | cut -f1 -d' '`
	echo $tool $loc
done >${PLOTSDIR}/fsltool-sloc.dat

# stack depth
echo "Scan depths..."
for fs in $FSNAMES; do
	get_display
	echo "#$fs" >${PLOTSDIR}/stackdepth.$fs.dat
	fname="${FSL_BASE}/tests/scantool-$fs/$fs-depth-1.img.stackstat"
	first=`grep bytes $fname | cut -f2 -d' '`
	for dnum in 1 4 16 64 128 192 256; do
		fname="${FSL_BASE}/tests/scantool-$fs/$fs-depth-${dnum}.img.stackstat"
		raw_bytes=`grep bytes $fname | cut -f2 -d' '`
		#echo $dnum `bc -l <<< "$raw_bytes/$first"`
		echo $dnum $raw_bytes
	done >>${PLOTSDIR}/stackdepth.$fs.dat
done

echo "Runtime SLOCs..."
sloccount ${FSL_BASE}/src/runtime/* | grep -A1 SLOC  | head -n2 | tail -n1 | cut -f1 -d' '	\
	>${PLOTSDIR}/runtime-lines.dat

echo "Compiler SLOCs..."
rm -f ${FSL_BASE}/src/compiler/parser.cc
rm -f ${FSL_BASE}/src/compiler/lex.yy.cc
sloccount ${FSL_BASE}/src/compiler/*.{cc,h,y} | grep -A1 SLOC  | head -n2 | tail -n1 | cut -f1 -d' '	\
	>${PLOTSDIR}/compiler-lines.dat

echo "Stat Breakdowns..."
util/plot/get_stack_breakdown.sh >${PLOTSDIR}/stack.dat

echo "OProf Breakdown..."
util/plot/get_oprof_breakdown.sh >${PLOTSDIR}/oprof.dat

# get iocache misses from .stats file
function grab_stat
{
	statname="$1"
	statfname="$2"
	if [ ! -e "$statfname" ]; then
		echo -n '-'
	else
	        grep "$statname" "$statfname" | cut -f4 -d' '
	fi
}

function tool_stat
{
	# Get cache misses
	TOOLNAMES="scan reloc scatter defrag"
	statname="$1"
	outfile="$2"
	echo "# $statname FOR DEFAULT TOOL RUNS" >"$outfile"
	echo "Filesystem $TOOLNAMES">>"$outfile"
	for fs in $FSNAMES; do
		l=""
		l="$l "`grab_stat $statname ${FSL_BASE}/tests/scantool-$fs/$fs-postmark.img.stats`
		l="$l "`grab_stat $statname ${FSL_BASE}/tests/relocate-$fs/$fs-relocate.img.stats`
		l="$l "`grab_stat $statname ${FSL_BASE}/tests/scattertool-$fs/$fs-scatter.img.stats`
		l="$l "`grab_stat $statname ${FSL_BASE}/tests/defragtool-$fs/$fs-defrag.img.stats`
		echo "$fs $l">>"$outfile"
	done
}

# $1 = fs
# $2 = tool
function test_misses_fname
{
	fs="$1"
	tool="$2"
	stat_fname="${FSL_BASE}/tests/${tool}*-$fs/${fs}-${tool}*.img.misses"
	stat_pm1_fname="${FSL_BASE}/tests/${tool}*-$fs/${fs}-${tool}*postmark*.img.misses"
	stat_pm_fname="${FSL_BASE}/tests/${tool}*-$fs/${fs}-postmark.img.misses"
	if [ -f $stat_pm1_fname ]; then
		echo -n $stat_pm1_fname
	elif [ -f $stat_fname ]; then
		echo -n $stat_fname
	elif [ -f $stat_pm_fname ]; then
		echo -n $stat_pm_fname
	else
		echo "NOTFOUND"
	fi
}

function test_stat_fname
{
	fs="$1"
	tool="$2"
	stat_fname="${FSL_BASE}/tests/${tool}*-$fs/${fs}-${tool}*.img.stats"
	stat_pm1_fname="${FSL_BASE}/tests/${tool}*-$fs/${fs}-${tool}*postmark*.img.stats"
	stat_pm_fname="${FSL_BASE}/tests/${tool}*-$fs/${fs}-postmark.img.stats"
	if [ -f $stat_pm1_fname ]; then
		echo -n $stat_pm1_fname
	elif [ -f $stat_fname ]; then
		echo -n $stat_fname
	elif [ -f $stat_pm_fname ]; then
		echo -n $stat_pm_fname
	else
		echo "NOTFOUND"
	fi
}

function stack_stat
{
	TOOLNAMES="scan relocate scatter defrag"
	statnames="$1"
	statprettynames="$2"
	outfile="$3"
	echo "# STACKSTAT FOR $statnames" >"$outfile"
	echo "Filesystem $statprettynames">>"$outfile"
	for fs in $FSNAMES; do
		for tool in $TOOLNAMES; do
			fname=`test_stat_fname $fs $tool`
			echo -n "$fs-$tool "
			for statname in $statnames; do
				echo -n " "`grab_stat "$statname" "$fname"`
			done
			echo
		done
		echo "-"
	done >>"$outfile"
}

function compulsory_stat
{
	TOOLNAMES="scan relocate scatter defrag"
	statprettynames="$2"
	outfile="${PLOTSDIR}/compulsory.dat"
	echo "# COMPULSORY" >"$outfile"
	echo "Filesystem Cold Conflict Compulsory Total">>"$outfile"
	for fs in $FSNAMES; do
		for tool in $TOOLNAMES; do
			fname=`test_misses_fname $fs $tool`
			statfname=`test_stat_fname $fs $tool`
			echo -n "$fs-$tool "
			if [ ! -e "$fname" ]; then
				compulsory="0"
				totalmiss="0"
				conflict="0"
				cold="0"
			else
				compulsory=`grep ": 1}" $fname | wc -l | cut -f1 -d' '`
				totalmiss=`grab_stat iocache_miss $statfname`
				cold=`wc -l $fname | cut -f1 -d' '`
				conflict=`bc <<<"$totalmiss - ( - $compulsory + $cold )"`
			fi
			echo "$cold $conflict $compulsory $totalmiss"
		#	echo $conflict
		done
		echo "-"
	done >>"$outfile"
}

# arg1 = real/user/sys
# arg2 = *.time file name
function get_total_seconds
{
	timeprefix="$1"
	benchtype="$2"
	sec_fname="tests/misc/$fs-${benchtype}.time"
	if [ ! -e "$sec_fname" ]; then
		echo -n "0"
		return
	fi
	timedat=`grep $timeprefix $sec_fname | cut -f2`
	time_m=`echo $timedat | cut -f1 -d'm'`
	time_s=`echo $timedat  | cut -f2 -d'm' | cut -f1 -d's'`
	echo -n `bc <<< "$time_m* 60 + $time_s"`
}

function time_stat
{
	stype="$1"
	echo "Filesystem	Native-fsck Native-du FSL-scan">${PLOTSDIR}/scan-$stype.dat
	for fs in $FSNAMES; do
		fsck_tots=`get_total_seconds "$stype" "fsck"`
		du_tots=`get_total_seconds "$stype" "du"`
		fsl_tots=`get_total_seconds "$stype" "fsl"`
		echo $fs $fsck_tots $du_tots $fsl_tots
	done >>${PLOTSDIR}/scan-$stype.dat
}

echo "Tool cache misses..."
tool_stat "iocache_miss" "${PLOTSDIR}/misses.dat"
echo "Tool cache hits..."
tool_stat "iocache_hit" "${PLOTSDIR}/hits.dat"
echo "Tool hit-miss..."
stack_stat "iocache_miss iocache_hit" "Miss Hit" "${PLOTSDIR}/hit-miss.dat"

echo "Compulsory misses...."
compulsory_stat

echo "Scan comparison test..."
time_stat "real"
time_stat "sys"
time_stat "user"

# MAKE FIGURES FOR PAPER
echo "Make the figs..."
gnuplot <util/plot/gnuplot/stack.gnu
gnuplot <util/plot/gnuplot/stackdepth.gnu
gnuplot <util/plot/gnuplot/scan.gnu
gnuplot <util/plot/gnuplot/frag.gnu
gnuplot <util/plot/gnuplot/oprof.gnu
gnuplot <util/plot/gnuplot/hits.gnu
gnuplot <util/plot/gnuplot/misses.gnu
gnuplot <util/plot/gnuplot/hit-miss.gnu

echo OK

echo "Making IO figs..."
gnuplot <util/plot/gnuplot/fsckio.gnu
gnuplot <util/plot/gnuplot/scanio.gnu
echo OK

echo "Building git stat chart."
util/gitgraph.sh
