#!/bin/bash

src_root=`pwd`

if [ ! -f tests.log ]; then
	echo "NO TESTLOG?"
	exit -1
fi

#last_commit=`git log --date=short | head -n3 | tail -n1 | cut -f2 -d':'`
d=`date "+%Y-%m-%d-%H:%M.stats"`

v=0
mkdir "${src_root}/metrics/$d/"
while read l; do
	v_name=`printf "%0#10d" $v`
	FSL_ENV_STATFILE="${src_root}/metrics/$d/${v_name}"
	export FSL_ENV_STATFILE
	eval $l
	v=`expr $v + 1`
done < "tests.log"
cp tests.log "${src_root}/metrics/$d/tests.log"
