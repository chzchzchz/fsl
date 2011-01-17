#!/bin/bash

src_root=`pwd`
export src_root

function fail_exit
{
	retval=$?
	if [ $retval -ne 0 ]; then
		echo "$1"
		echo "!!!FAILED: $2"
		exit $retval
	fi
}

function fail_exit_file
{
	retval=$?
	if [ $retval -ne 0 ]; then
		cat "$1"
		echo "!!!FAILED: $2"
		exit $retval
	fi
}

function force_fail_exit
{
# do not jump into debugger
	rm -f failed_test_cmd
	echo "$1"
	echo "!!!FAILED: $2"
	exit -1
}

function force_fail_exit_file
{
# do not jump into debugger
	rm -f failed_test_cmd
	cat "$1"
	echo "!!!FAILED: $2"
	exit -1
}

function run_test
{
	TESTNUM="$1"
	TESTNAME="$2"

	cmd="${src_root}/bin/${tool_name} ${src_root}/img/${img_name} 2>&1 <${src_root}/tests/${tool_name}/test-input-$TESTNUM"
	echo "$cmd" >failed_test_cmd
	echo "$cmd" >>tests.log
	eval $cmd >run_test.out
	fail_exit run_test.out "$TESTNAME"
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
	force_fail_exit "$test_outstr" "$TESTNAME"
}

function run_test_match
{
	local badmatch="$3"
	local TESTNUM="$1"
	local TESTNAME="$2"

	run_test "$1" "$2"
	matched=`grep "$badmatch" run_test.out`
	if [ -z "$matched" ]; then
		force_fail_exit_file run_test.out "$2"
	fi
}
