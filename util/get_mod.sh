#!/bin/sh

LOGINUSER=chz
LOGINHOST=10.0.0.10
LOGINSSH=$LOGINUSER@$LOGINHOST
LOGINFSLROOT=/home/chz/fs/

if [ -z "$FSNAME" ]; then
	echo FSNAME env var not given.
	exit 1
fi

case $FSNAME in
ext2)		DEVNAME=/dev/sdb 	;;
vfat)		DEVNAME=/dev/sdc	;;
iso9660)	DEVNAME=/dev/sdd	;;
reiserfs)	DEVNAME=/dev/sde	;;
xfs)		DEVNAME=/dev/sdf	;;
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
