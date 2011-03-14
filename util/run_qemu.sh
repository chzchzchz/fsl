#!/bin/bash

IMGBASE=img/

if [ -z "$VMIMG" ]; then
	VMIMG=~/src/img/fsl-vm.img
fi

if [ -z $RAM_MB ]; then
	RAM_MB=1024
fi

TESTIMGS="ext2-postmark.img
	vfat-postmark.img
	iso9660-postmark.img
	reiserfs-postmark.img
	xfs-postmark.img"

for a in $TESTIMGS; do
	rm -f $IMGBASE/$a.qcow2
	qemu-img convert -O qcow2 $IMGBASE/$a $IMGBASE/$a.qcow2
done

#qemu-img convert -O qcow2 fsl-vm.img fsl-vm.img.qcow2
kvm 										\
	-nographic 								\
	-usb -usbdevice mouse							\
	-smp 4 -m $RAM_MB  -hda $VMIMG 						\
	-drive file=$IMGBASE/ext2-postmark.img.qcow2				\
	-drive file=$IMGBASE/vfat-postmark.img.qcow2,media=disk,if=scsi	\
	-drive file=$IMGBASE/iso9660-postmark.img.qcow2,media=disk,if=scsi	\
	-drive file=$IMGBASE/reiserfs-postmark.img.qcow2,media=disk,if=scsi	\
	-drive file=$IMGBASE/xfs-postmark.img.qcow2,media=disk,if=scsi	\
	-net tap,vlan=0,ifname=tap1,script=./util/qemu-ifup.sh	\
	-net nic,vlan=0,macaddr=de:ad:be:ee:EE:ffa
