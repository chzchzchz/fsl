#!/bin/bash

# unmount fs, deassociate image with lo
sudo /bin/umount /mnt/FSTEST
sudo sync
retval=$?
if [ $retval -ne 0 ]; then
	echo "Failed to unmount."
fi

if [ -z $LODEV ]; then
	LODEV=/dev/loop5
fi


sudo /sbin/losetup -d $LODEV
retval=$?

if [ $retval -ne 0 ]; then
	echo "Failed to losetup -d"
fi

exit $retval
