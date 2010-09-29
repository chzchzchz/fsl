#!/bin/bash

# unmount fs, deassociate image with lo
sudo /bin/umount /mnt/FSTEST
retval=$?
if [ $retval -ne 0 ]; then
	echo "Failed to unmount."
fi

sudo /sbin/losetup -d /dev/loop5
retval=$?

if [ $retval -ne 0 ]; then
	echo "Failed to losetup -d"
fi

exit $retval
