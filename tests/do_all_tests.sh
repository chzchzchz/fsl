#!/bin/bash

function browser_startup
{
	fs=$1
	echo "Testing browser-$fs startup."
	cmd="${src_root}/src/tool/browser-$fs ${src_root}/img/$fs.img <<<EOF"
	echo "$cmd" >>tests.log
	outstr=`eval $cmd`
	retval=$?
	if [ $retval -ne 0 ]; then
		echo "Test failed: $fs."
		echo "Output: '$outstr'"
		exit $retval
	fi
}

function specific_tests
{
	fs=$1
	${src_root}/tests/do_tests_$fs.sh
	retval=$?
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
make  1>/dev/null
makeret=$?
if [ $makeret -ne 0 ]; then
	echo "Tests failed. Could not build tools."
	exit $makeret
fi

cd "${src_root}"

rm -f failed_test_cmd
rm -f tests.log

for a in ext2 vfat nilfs; do
	browser_startup $a
	specific_tests $a
done

rm -f failed_test_cmd

exit 0