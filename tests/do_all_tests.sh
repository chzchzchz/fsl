#!/bin/bash

function browser_startup
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

function scan_startup_img
{
	fs="$1"
	imgname="$2"
	echo "Testing scantool-$fs startup."
	cmd="${src_root}/src/tool/scantool-$fs ${src_root}/img/$imgname"
	echo "$cmd" >>tests.log
	echo "$cmd" >failed_test_cmd
	eval "$cmd >cur_test.out"
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

function scan_startup
{
	FS="$1"
	scan_startup_img "$FS" "${FS}.img"
}

function specific_tests
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


# build
src_root=`pwd`

echo "Pre-commit Tests..."
echo "Building language."

cd "${src_root}/src/"
make -j6  1>/dev/null
makeret=$?
if [ $makeret -ne 0 ]; then
	echo "Tests failed. Could not build lang."
	echo "Error: $makeret"
	exit $makeret
fi

echo "Building tools."

cd "${src_root}/src/tool"
make -j6  1>/dev/null
makeret=$?
if [ $makeret -ne 0 ]; then
	echo "Tests failed. Could not build tools."
	exit $makeret
fi

cd "${src_root}"

rm -f failed_test_cmd
rm -f tests.log

scan_startup_img ext2 ext2-small.img
for a in ext2 vfat nilfs2 "testfs"; do
	scan_startup "$a"
	browser_startup "$a"
	specific_tests "$a"
done

rm -f failed_test_cmd

exit 0