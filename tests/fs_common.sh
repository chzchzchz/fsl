#!/bin/bash

#env variables:
# USE_OPROF
# USE_STATS
# XFM_PIN_STACK
# XFM_MEM

if [ -z $src_root ]; then
	echo "Source root not defined!"
	exit -1
fi

if [ -z $TOOL_RT ]; then
	TOOL_BINDIR="$src_root/bin/"
else
	TOOL_BINDIR="$src_root/bin/mmap/"
fi

function start_oprof
{
	if [ ! -z $USE_OPROF ]; then
		sudo opcontrol --start --vmlinux=/usr/src/linux/vmlinux --event=CPU_CLK_UNHALTED:400000:0:1:1
	fi
}

function stop_oprof
{
	if [ ! -z $USE_OPROF ]; then
		oproffile="$1".oprof
		cmdname="$2"
		sudo opcontrol --dump
		sudo opreport -g -a --symbols "$cmdname" >${oproffile}
		sudo opcontrol --stop
		sudo opcontrol --reset
	fi
}

function set_statfiles
{
	fnameprefix="$1"
	if [ ! -z $USE_STATS ]; then
		export FSL_ENV_HITFILE="${fnameprefix}.hits"
		export FSL_ENV_MISSFILE="${fnameprefix}.misses"
		export FSL_ENV_STATFILE="${fnameprefix}.stats"
		if [ ! -z $2 ]; then
			export FSL_ENV_WRITEFILE="${fnameprefix}.writes"
		fi
	fi
}

function unset_statfiles
{
	unset FSL_ENV_HITFILE
	unset FSL_ENV_MISSFILE
	unset FSL_ENV_STATFILE
	unset FSL_ENV_WRITEFILE
	unset LD_PRELOAD
	unset MMAP_INST_OUTFNAME
}

function get_timefname
{
	timefnameprefix="$1"

	if [ ! -z $USE_OPROF ]; then
		TIMEFNAME_VAL="${timefnameprefix}.oprof.time"
	elif [ ! -z $XFM_PIN_STACK ]; then
		TIMEFNAME_VAL="${timefnameprefix}.stack.time"
	elif [ ! -z $XFM_MEM ]; then
		TIMEFNAME_VAL="${timefnameprefix}.mem.time"
	else
		TIMEFNAME_VAL="${timefnameprefix}.time"
	fi
}

function cmd_xfrm
{
	OLDCMD="$1"
	FPRE="$2"

	NEWCMD="$OLDCMD"
	if [ ! -z $XFM_PIN_STACK ]; then
		NEWCMD="${src_root}/lib/pin/pin -t ${src_root}/lib/pin/source/tools/chz/obj-intel64/stackuse.so -o ${FPRE}.stackstat -- $OLDCMD"
	fi

	if [ ! -z $XFM_MEM ]; then
		LD_PRELOAD="${src_root}/lib/mmap_inst/mmap_inst.so:${src_root}/lib/malloc/libptmalloc3.so"
		MMAP_INST_OUTFNAME="${FPRE}.memstat"
		export MMAP_INST_OUTFNAME
		export LD_PRELOAD
	fi

	CMD_XFM_VAL="$NEWCMD"
}

function fs_browser_startup
{
	fs="$1"
	echo "Testing browser-$fs startup."
	cmd="${TOOL_BINDIR}/browser-$fs ${src_root}/img/$fs.img <<<EOF"
	echo "$cmd" >>tests.log
	echo "$cmd" >failed_test_cmd
	outstr=`eval $cmd`
	retval=$?
	if [ $retval -ne 0 ]; then
		echo "Test failed: $fs."
		echo "Output: '$outstr'"
		exit $retval
	fi
}

function fs_reloc_img
{
	fs="$1"
	imgname="$2"
	picname="$3"

	echo "Testing relocate-$fs (${imgname} => ${picname})."
	cmd="${TOOL_BINDIR}/relocate-$fs ${src_root}/img/$imgname ${picname}"
	outdir="${src_root}/tests/relocate-$fs"
	fs_cmd_startup_img "$cmd" "$outdir" "$imgname" "WRITE"
}

function fs_reloc_startup_img
{
	fs_reloc_img "$1" "$2" "${src_root}/tests/reloc.spock.pic"
}

function fs_smush_startup_img
{
	fs="$1"
	imgname="$2"
	echo "Testing smushtool-$fs startup (${imgname})."
	cmd="${TOOL_BINDIR}/smushtool-$fs ${src_root}/img/$imgname"
	outdir="${src_root}/tests/smushtool-$fs"
	fs_cmd_startup_img "$cmd" "$outdir" "$imgname" "WRITE"
}

function fs_scatter_startup_img
{
	fs="$1"
	imgname="$2"
	echo "Testing scattertool-$fs startup (${imgname})."
	cmd="${TOOL_BINDIR}/scattertool-$fs ${src_root}/img/$imgname"
	outdir="${src_root}/tests/scattertool-$fs"
	fs_cmd_startup_img "$cmd" "$outdir" "$imgname" "WRITE"
}

function fs_thrashcopy_startup_img
{
	fs="$1"
	imgname="$2"
	echo "Testing thrashcopy-$fs startup (${imgname})."
	cmd="${TOOL_BINDIR}/thrashcopy-$fs ${src_root}/img/$imgname"
	outdir="${src_root}/tests/thrashcopy-$fs"
	fs_cmd_startup_img "$cmd" "$outdir" "$imgname" "WRITE"
}


function fs_defrag_startup_img
{
	fs="$1"
	imgname="$2"
	echo "Testing defragtool-$fs startup (${imgname})."
	cmd="${TOOL_BINDIR}/defragtool-$fs ${src_root}/img/$imgname"
	outdir="${src_root}/tests/defragtool-$fs"
	fs_cmd_startup_img "$cmd" "$outdir" "$imgname" "WRITE"
}

function fs_scan_startup_img
{
	fs="$1"
	imgname="$2"
	echo "Testing scantool-$fs startup (${imgname})."
	cmd="${TOOL_BINDIR}/scantool-$fs ${src_root}/img/$imgname"
	outdir="${src_root}/tests/scantool-$fs"
	fs_cmd_startup_img "$cmd" "$outdir" "$imgname"
}

function fs_cmd_startup_img
{
	cmd="$1"
	outdir="$2"
	imgname="$3"
	doeswrite="$4"

	SHANAME=`echo -n "$cmd" | shasum | cut -f1 -d' '`
	echo "$cmd" >>tests.log
	echo "$cmd" >failed_test_cmd

	fprefix="${outdir}/${imgname}"
	start_oprof
	set_statfiles "$fprefix" "$doeswrite"
	get_timefname "$fprefix"
	timefname=${TIMEFNAME_VAL}
	cmd_xfrm "$cmd" "$fprefix"
	{ time eval "${CMD_XFM_VAL}" >cur_test.${SHANAME}.out 2>cur_test.${SHANAME}.err; } 2>${timefname}
	retval=$?
	unset_statfiles
	stop_oprof "$fprefix" `echo $cmd | cut -f1 -d' ' `

	if [ $retval -ne 0 ]; then
		echo "Test failed: $fs."
		echo "Output: "
		echo "-------------"
		cat cur_test.${SHANAME}.out
		echo "-------------"
		exit $retval
	fi

	# only copy if result is not bogus from crash
	mv cur_test.${SHANAME}.out "${outdir}/$imgname.out"
	mv cur_test.${SHANAME}.err "${outdir}/$imgname.err"
}


function fs_scan_startup
{
	FS="$1"
	fs_scan_startup_img "$FS" "${FS}.img"
}

function fs_specific_tests
{
	fs="$1"
	cmd_script="${src_root}/tests/do_tests_${fs}.sh"
	if [ ! -f "$cmd_script" ]; then
		echo "***No SPECIFIC tests for $fs"
		return
	fi

	$cmd_script
	retval="$?"
	if [ $retval -ne 0 ]; then
		echo "Test failed: $fs."
		exit $retval
	fi
}
