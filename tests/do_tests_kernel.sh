#!/bin/bash
if [ -z $FSNAME ]; then
	FSNAME=ext2
fi
scp util/get_mod.sh root@10.0.0.100:/root
scp tests/tests_kern.run.$FSNAME.sh root@10.0.0.100:/root
ssh root@10.0.0.100 "FSNAME=$FSNAME /root/get_mod.sh" >tests/kernel-$FSNAME/ssh.out
exec tests/tests_kern.verify.$FSNAME.sh
#killall qemu-system-x86_64