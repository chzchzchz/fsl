#!/bin/bash

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

function fs_scan_startup_img
{
	fs="$1"
	imgname="$2"
	echo "Testing scantool-$fs startup (${imgname})."
	cmd="${src_root}/src/tool/scantool-$fs ${src_root}/img/$imgname"
	echo "$cmd" >>tests.log
	echo "$cmd" >failed_test_cmd
	export FSL_ENV_HITFILE="${src_root}/tests/scantool-$fs/${imgname}.hits"
	eval "$cmd >cur_test.out"
	unset FSL_ENV_HITFILE
	retval=$?
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
