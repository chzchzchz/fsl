#!/bin/bash
# build

src_root=`pwd`
FILESYSTEMS="ext2 vfat nilfs2 testfs iso9660"

echo "Pre-commit Tests..."
echo "Building language."

cd "${src_root}/"
make tools  1>/dev/null
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

export src_root
source ${src_root}/tests/fs_common.sh

rm -f failed_test_cmd
rm -f tests.log

if [ "$TEST_CONFIG" == "EXTRA" ]; then
	echo "EXTRA TESTS!"
	fs="$TEST_FS"

	if [ ! -z $USE_OPROF ]; then
		export USE_OPROF
	fi

	if [ ! -z $USE_STATS ]; then
		export USE_STATS
	fi

	${src_root}/tests/tests_extra.$fs.sh
	ret=$?
	if [ $ret -ne 0 ]; then
		echo "$fs: extras falied."
		exit $ret
	fi
elif [ "$TEST_CONFIG" == "STACK" ]; then
	echo "STACK TESTS!"
	fs="$TEST_FS"

	unset USE_OPROF
	unset USE_STATS
	export XFM_PIN_STACK="YES"

	${src_root}/tests/tests_extra.$fs.sh
	ret=$?
	if [ $ret -ne 0 ]; then
		echo "$fs: extras falied."
		exit $ret
	fi
elif [ "$TEST_CONFIG" == "MEM" ]; then
	echo "MEM TESTS!"
	fs="$TEST_FS"

	unset USE_OPROF
	unset USE_STATS
	export XFM_MEM="YES"

	${src_root}/tests/tests_extra.$fs.sh
	ret=$?
	if [ $ret -ne 0 ]; then
		echo "$fs: extras falied."
		exit $ret
	fi
else
	echo "STANDARD TESTS"
	export USE_STATS="YES"
	for fs in $FILESYSTEMS; do
		fs_browser_startup "$fs"
		fs_specific_tests "$fs"
		fs_scan_startup "$fs"
	done
	fs_scan_startup_img ext2 ext2-small.img
fi

rm -f failed_test_cmd

exit 0