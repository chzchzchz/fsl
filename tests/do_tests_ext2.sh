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

function run_test
{
	TESTNUM="$1"
	TESTNAME="$2"

	cmd="${src_root}/src/tool/browser-ext2 ${src_root}/img/ext2.img 2>&1 <${src_root}/tests/ext2-test-input-$TESTNUM"
	echo "$cmd" >failed_test_cmd
	echo "$cmd" >>tests.log
	outstr=`eval $cmd`
	fail_exit "$outstr" "$TESTNAME"
}


# visit virtual file (inode 0)
run_test 1 "Virtual Visit"
run_test 2 "Virtual Dump"

exit 0
