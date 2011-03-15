#!/bin/bash

if [ -z $1 ]; then
	echo "Expects library."
fi

code=`objdump -x $1 | grep ".text " | awk '{ print $3; }'`
code=`printf "%d" 0x"$code"`
data=`objdump -x $1 | grep "\.data " | awk '{ print $3; }'`
data=`printf "%d" 0x"$data"`
rodata=`objdump -x $1 | grep "\.rodata " | awk '{ print $3; }'`
rodata=`printf "%d" 0x"$rodata"`

echo "CODE: " $code
echo "DATA: " $data
echo "RODATA: " $rodata
echo "TOTAL: " `expr $code + $data + $rodata`
