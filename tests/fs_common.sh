#!/bin/bash

function start_oprof
{
	sudo opcontrol --start --vmlinux=/usr/src/linux/vmlinux
}

function stop_oprof
{
	oproffile="$1"
	echo "NOT MOVING"
	sudo opcontrol --dump
	sudo opreport -a --symbols >${oproffile}
	sudo opcontrol --stop
	sudo opcontrol --reset
}

function fs_browser_startup
{
	fs="$1"
	echo "Testing browser-$fs startup."
	cmd="${src_root}/src/tool/browser-$fs ${src_root}/img/$fs.img <<<EOF"
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

function fs_reloc_startup_img
{
	fs="$1"
	imgname="$2"
	echo "Testing reloc-$fs spockup ($imgname)."

	cmd="${src_root}/src/tool/relocate-$fs ${src_root}/img/$imgname ${src_root}/tests/reloc.spock.pic"
	echo "$cmd" >>tests.log
	echo "$cmd" >failed_test_cmd

	if [ ! -z $OPROFILE_FLAG ]; then
		start_oprof
		timefname="${src_root}"/tests/relocate-$fs/$imgname.reloc.oprof.time
	else
		export FSL_ENV_HITFILE="${src_root}/tests/relocate-$fs/${imgname}.hits"
		export FSL_ENV_MISSFILE="${src_root}/tests/relocate-$fs/${imgname}.misses"
		export FSL_ENV_STATFILE="${src_root}/tests/relocate-$fs/${imgname}.stats"
		export FSL_ENV_WRITEFILE="${src_root}/tests/relocate-$fs/${imgname}.writes"
		timefname="${src_root}"/tests/relocate-$fs/$imgname.reloc.time
	fi
	{ time eval "$cmd" >cur_test.out 2>cur_test.err; } 2>${timefname}
	retval=$?

	unset FSL_ENV_HITFILE
	unset FSL_ENV_MISSFILE
	unset FSL_ENV_WRITEFILE
	unset FSL_ENV_STATFILE
	if [ ! -z $OPROFILE_FLAG ]; then
		stop_oprof "${src_root}"/tests/relocate-$fs/$imgname.reloc.oprof
	fi

	if [ $retval -ne 0 ]; then
		echo "Test failed: $fs."
		echo "Output: "
		echo "-------------"
		cat cur_test.out
		echo "-------------"
		exit $retval
	fi
	# only copy if result is not bogus from crash
	cp cur_test.out "${src_root}"/tests/relocate-$fs/$imgname.reloc.out
}

function fs_defrag_startup_img
{
	fs="$1"
	imgname="$2"
	echo "Testing defragtool-$fs startup ($imgname)."

	cmd="${src_root}/src/tool/defragtool-$fs ${src_root}/img/$imgname"
	echo "$cmd" >>tests.log
	echo "$cmd" >failed_test_cmd

	if [ ! -z $OPROFILE_FLAG ]; then
		start_oprof
		timefname="${src_root}"/tests/defragtool-$fs/$imgname.defrag.oprof.time
	else
		export FSL_ENV_HITFILE="${src_root}/tests/defragtool-$fs/${imgname}.hits"
		export FSL_ENV_MISSFILE="${src_root}/tests/defragtool-$fs/${imgname}.misses"
		export FSL_ENV_WRITEFILE="${src_root}/tests/defragtool-$fs/${imgname}.writes"
		export FSL_ENV_STATFILE="${src_root}/tests/defragtool-$fs/${imgname}.stats"
		timefname="${src_root}"/tests/defragtool-$fs/$imgname.defrag.time
	fi
	{ time eval "$cmd" >cur_test.out 2>cur_test.err; } 2>${timefname}
	retval=$?

	unset FSL_ENV_HITFILE
	unset FSL_ENV_MISSFILE
	unset FSL_ENV_WRITEFILE
	unset FSL_ENV_STATFILE
	if [ ! -z $OPROFILE_FLAG ]; then
		stop_oprof "${src_root}"/tests/defragtool-$fs/$imgname.defrag.oprof
	fi

	if [ $retval -ne 0 ]; then
		echo "Test failed: $fs."
		echo "Output: "
		echo "-------------"
		cat cur_test.out
		echo "-------------"
		exit $retval
	fi
	# only copy if result is not bogus from crash
	cp cur_test.out "${src_root}"/tests/defragtool-$fs/$imgname.defrag.out
}

function fs_scan_startup_img
{
	fs="$1"
	imgname="$2"
	echo "Testing scantool-$fs startup (${imgname})."
	cmd="${src_root}/src/tool/scantool-$fs ${src_root}/img/$imgname"
	echo "$cmd" >>tests.log
	echo "$cmd" >failed_test_cmd

	if [ ! -z $OPROFILE_FLAG ]; then
		start_oprof
		timefname="${src_root}"/tests/scantool-$fs/$imgname.scan.oprof.time
	else
		export FSL_ENV_HITFILE="${src_root}/tests/scantool-$fs/${imgname}.hits"
		export FSL_ENV_MISSFILE="${src_root}/tests/scantool-$fs/${imgname}.misses"
		export FSL_ENV_STATFILE="${src_root}/tests/scantool-$fs/${imgname}.stats"
		timefname="${src_root}"/tests/scantool-$fs/$imgname.scan.time
	fi
	{ time eval "$cmd" >cur_test.out 2>cur_test.err; } 2>${timefname}
	retval=$?

	unset FSL_ENV_HITFILE
	unset FSL_ENV_MISSFILE
	unset FSL_ENV_STATFILE
	if [ ! -z $OPROFILE_FLAG ]; then
		stop_oprof "${src_root}"/tests/scantool-$fs/$imgname.scan.oprof
	fi

	if [ $retval -ne 0 ]; then
		echo "Test failed: $fs."
		echo "Output: "
		echo "-------------"
		cat cur_test.out
		echo "-------------"
		exit $retval
	fi
	# only copy if result is not bogus from crash
	cp cur_test.out "${src_root}"/tests/scantool-$fs/$imgname.scan.out

}

function fs_scan_startup
{
	FS="$1"
	fs_scan_startup_img "$FS" "${FS}.img"
}

function fs_specific_tests
{
	fs="$1"
	cmd_script="${src_root}/tests/do_tests_$fs.sh"

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
