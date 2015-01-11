#!/bin/bash
# build

src_root=`pwd`

echo "Tests.."

export src_root
source ${src_root}/tests/fs_common.sh

rm -f failed_test_cmd
rm -f tests.log

function try_extra
{
	if [ "$TEST_CONFIG" != "EXTRA" ]; then return 0; fi
	echo "EXTRA TESTS!"
	fs="$TEST_FS"

	if [ ! -z $USE_OPROF ]; then export USE_OPROF; fi
	if [ ! -z $USE_STATS ]; then export USE_STATS; fi

	${src_root}/tests/tests_extra.$fs.sh
	ret=$?
	if [ $ret -ne 0 ]; then
		echo "$fs: extras falied."
		return 1
	fi

	return 0
}

function try_stack
{
	if [ "$TEST_CONFIG" != "STACK" ]; then return 0; fi
	echo "STACK TESTS!"
	fs="$TEST_FS"

	unset USE_OPROF
	unset USE_STATS
	export XFM_PIN_STACK="YES"

	${src_root}/tests/tests_extra.$fs.sh
	ret=$?
	if [ $ret -ne 0 ]; then
		echo "$fs: extras falied."
		return 1
	fi
	return 0
}

function try_mem
{
	if [ "$TEST_CONFIG" != "MEM" ]; then return 0; fi
	echo "MEM TESTS!"
	fs="$TEST_FS"

	unset USE_OPROF
	unset USE_STATS
	export XFM_MEM="YES"

	${src_root}/tests/tests_extra.$fs.sh
	ret=$?
	if [ $ret -ne 0 ]; then
		echo "$fs: extras falied."
		return 1
	fi
	return 0
}

function do_std_fs
{
	if [ ! -z "$TEST_CONFIG" ]; then return 0; fi
	echo "STANDARD TESTS!"
	fs="$TEST_FS"

	export USE_STATS="YES"
	fs_browser_startup "$fs"
	fs_scan_startup "$fs"
	fs_specific_tests "$fs"
	# XXX dumb
	if [ "$fs" == "ext2" ]; then
		fs_scan_startup_img ext2 ext2-small.img
	fi
	return 0
}

if [ ! -z "$FILESYSTEMS" ]; then
	for fs in $FILESYSTEMS; do
		TEST_FS="$fs"
		try_extra && try_stack && try_mem && do_std_fs
		# rm -f failed_test_cmd
	done
elif [ ! -z "$TEST_FS" ]; then
	try_extra && try_stack && try_mem && do_std_fs
else
	echo dont know what fs to test...
fi