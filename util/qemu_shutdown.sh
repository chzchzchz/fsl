#!/bin/bash

ssh root@10.0.0.100 "shutdown -h now"
x=1
while [ 1 ]; do
	sleep 1
	qemu_there=`ps -C qemu-system-x86_64 -o pid=`
	if [ -z "$qemu_there" ]; then
		break
	fi
	echo "Waiting for QEMU Shutdown ...${x}s."
	x=`expr $x + 1`
done