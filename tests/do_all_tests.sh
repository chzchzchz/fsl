#!/bin/bash
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

export src_root
source ${src_root}/tests/fs_common.sh

rm -f failed_test_cmd
rm -f tests.log

fs_scan_startup_img ext2 ext2-small.img
for a in ext2 vfat nilfs2 testfs; do
	fs_scan_startup "$a"
	fs_browser_startup "$a"
	fs_specific_tests "$a"
done

if [ "$TEST_CONFIG" == "FULL" ]; then
	echo "FULL TEST!"
	fs=ext2
	imgname=ext2-many.img
	fs_scan_startup_img ext2 ext2-many.img
	unset FSL_ENV_HITFILE
fi

rm -f failed_test_cmd

exit 0