#!/bin/sh

if [ -z $FSNAME ]; then
	echo FSNAME env var not given.
	exit 1
fi

echo "Test Mode: $FSNAME"

rm -f fsl.ko
rm -f fslcmd
rm *.test
scp chz@10.0.0.10:/home/chz/src/research/fs/util/get_mod.sh ./
scp chz@10.0.0.10:/home/chz/src/research/fs/tests/tests_kern.run.$FSNAME.sh ./
scp chz@10.0.0.10:/home/chz/src/research/fs/src/kernel/mod/fsl.ko ./
scp chz@10.0.0.10:/home/chz/src/research/fs/bin/fslcmd ./

sudo insmod ./fsl.ko
sudo chmod 666 /dev/fslctl

echo "==========TOOLS=========="
sudo ./fslcmd get /dev/sdb
sudo ./fslcmd scatter
sudo ./fslcmd defrag
sudo ./fslcmd smush
sudo ./fslcmd put

grep fsl /proc/filesystems >proc.test

sudo mount -t fslfs-$FSNAME /dev/sdb /mnt/fslfs
./tests_kern.run.$FSNAME.sh
scp *.test chz@10.0.0.10:/home/chz/src/research/fs/tests/kernel-$FSNAME/
sudo umount /mnt/fslfs
sudo rmmod fsl

echo "=============DMESG============"
dmesg | tail -n200
