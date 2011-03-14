#!/bin/bash
if [ -z $FSNAME ]; then
	FSNAME=ext2
fi

TAPIP="10.0.0.10"
VMIP="10.0.0.100"
TAPADDR=`/sbin/ifconfig | grep $TAPIP`
if [ -z "$TAPADDR" ]; then
	echo TapAddr not set to $TAPIP
	exit 1
fi

x=1
while [ 1 ]; do
	ping -c1 -W 1 $VMIP 2>/dev/null >/dev/null
	retval=$?
	if [ $retval -eq 0 ]; then
		break
	fi
	echo "Waiting for KVM-$FSNAME... ${x}s"
	if [ $x -gt 30 ]; then
		echo "KVM-$FSNAME timed out."
		exit -3
	fi
	x=`expr $x + 1`
done


scp util/get_mod.sh root@$VMIP:/root
scp tests/tests_kern.run.$FSNAME.sh root@$VMIP:/root
ssh root@$VMIP "FSNAME=$FSNAME /root/get_mod.sh" >tests/kernel-$FSNAME/ssh.out
exec tests/tests_kern.verify.$FSNAME.sh
#killall qemu-system-x86_64