#!/bin/sh

#####
# NOTE: THIS RUNS INSIDE THE VM
######

LOGINUSER=chz
LOGINHOST=10.0.0.10
LOGINSSH=$LOGINUSER@$LOGINHOST
LOGINFSLROOT=/home/chz/fs/

if [ -z "$FSNAME" ]; then
	echo FSNAME env var not given.
	exit 1
fi

ext2dev=`ls /sys/bus/scsi/devices/target*\:0\:2/*\:0\:2\:0/block/`
vfatdev=`ls /sys/bus/scsi/devices/target*\:0\:3/*\:0\:3\:0/block/`
iso9660dev=`ls /sys/bus/scsi/devices/target*\:0\:4/*\:0\:4\:0/block/`
reiserfsdev=`ls /sys/bus/scsi/devices/target*\:0\:5/*\:0\:5\:0/block/`
xfsdev=`ls /sys/bus/scsi/devices/target*\:0\:6/*\:0\:6\:0/block/`
#dev=`ls /sys/bus/scsi/devices/target10\:0\:1/10\:0\:6\:0/block/`

case $FSNAME in
ext2)		DEVNAME=/dev/$ext2dev 		;;
vfat)		DEVNAME=/dev/$vfatdev		;;
iso9660)	DEVNAME=/dev/$iso9660dev	;;
reiserfs)	DEVNAME=/dev/$reiserfsdev	;;
xfs)		DEVNAME=/dev/$xfsdev		;;
*)
		echo "WHICH DEV FOR $FSNAME??"
		exit 100
		;;
esac

if [ -z "$DEVNAME" ]; then
	echo NO DEVNAME
	exit 2
fi

echo "Test Mode: $FSNAME"

rm -f fsl$FSNAME.ko
rm -f fslcmd
rm -f *.test
scp $LOGINSSH:$LOGINFSLROOT/util/get_mod.sh ./
scp $LOGINSSH:$LOGINFSLROOT/tests/tests_kern.run.$FSNAME.sh ./
scp $LOGINSSH:$LOGINFSLROOT/src/kernel/mod/fsl$FSNAME.ko ./
scp $LOGINSSH:$LOGINFSLROOT/bin/fslcmd ./

sudo insmod ./fsl$FSNAME.ko
if [ -z $? ]; then
	dmesg
	exit 1
fi

sudo chmod 666 /dev/fslctl

echo "==========TOOLS=========="
sudo ./fslcmd get $DEVNAME
sudo ./fslcmd scatter
sudo ./fslcmd defrag
sudo ./fslcmd smush
sudo ./fslcmd put

cat /proc/filesystems >proc.test
cat /proc/modules >modules.test

sudo mount -t fslfs-$FSNAME $DEVNAME /mnt/fslfs
./tests_kern.run.$FSNAME.sh
scp *.test $LOGINSSH:$LOGINFSLROOT/tests/kernel-$FSNAME/
sudo umount /mnt/fslfs
sudo rmmod fsl$FSNAME

echo "=============DMESG============"
dmesg | tail -n200
