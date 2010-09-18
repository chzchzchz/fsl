#!/bin/bash

src_root=`pwd`
function fail_exit
{
	retval=$?
	if [ $retval -ne 0 ]; then
		echo "$1"
		echo "!!!FAILED: $2"
		exit $retval
	fi
}

function run_test
{
	TESTNUM="$1"
	TESTNAME="$2"

	cmd="${src_root}/src/tool/browser-vfat ${src_root}/img/vfat.img 2>&1 <${src_root}/tests/vfat-test-input-$TESTNUM"
	echo "$cmd" >failed_test_cmd
	echo "$cmd" >>tests.log
	test_outstr=`eval $cmd`
	fail_exit "$test_outstr" "$TESTNAME"
}

function run_test_no_match
{
	local badmatch="$3"
	local TESTNUM="$1"
	local TESTNAME="$2"
	run_test "$TESTNUM" "$TESTNAME"
	matched=`echo "$test_outstr" | grep "$badmatch"`
	if [ -z "$matched" ]; then
		return
	fi
	fail_exit "$outstr" "$TESTNAME"
}

function run_test_match
{
	local badmatch="$3"
	local TESTNUM="$1"
	local TESTNAME="$2"

	run_test "$1" "$2"
	matched=`echo "$test_outstr" | grep "$badmatch"`
	if [ -z "$matched" ]; then
		fail_exit "$outstr" "$2"
	fi
}



run_test 1 "Visit Root DE[1]"
run_test_no_match 2 "Virt-If. Non-File" "vfile"
run_test_match 3 "Virt-If. File." "vfile"

exit 0
