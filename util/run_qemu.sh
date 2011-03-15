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
	-smp 4 -m $RAM_MB							\
	-drive file=$VMIMG,if=ide,index=0,media=disk,boot=on			\
	-drive file=$IMGBASE/ext2-postmark.img.qcow2,media=disk,index=2,if=scsi	\
	-drive file=$IMGBASE/vfat-postmark.img.qcow2,media=disk,index=3,if=scsi	\
	-drive file=$IMGBASE/iso9660-postmark.img.qcow2,media=disk,index=4,if=scsi	\
	-drive file=$IMGBASE/reiserfs-postmark.img.qcow2,media=disk,index=5,if=scsi \
	-drive file=$IMGBASE/xfs-postmark.img.qcow2,media=disk,if=scsi,index=6,if=scsi \
	-net tap,vlan=0,ifname=tap1,script=./util/qemu-ifup.sh						\
	-net nic,vlan=0,macaddr=de:ad:be:ee:EE:ffa
#	-drive file=$VMIMG,media=disk,bus=7,unit=1,if=scsi,boot=on		\
