#!/bin/bash

util/draw_scan.py tests/scantool-ext2/ext2-relocate.img.scan.processed.out `expr 1024 \* 102400` tests/scantool-ext2/ext2-spock.png 250 300
util/draw_scan.py tests/scantool-vfat/vfat-relocate.img.scan.processed.out `expr 1024 \* 102400` tests/scantool-vfat/vfat-spock.png 250 300