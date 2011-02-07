#!/bin/bash

util/draw_scan.py tests/scantool-ext2/ext2-relocate.img.processed.out `expr 1024 \* 102400` tests/scantool-ext2/ext2-spock.png 250 300
util/draw_scan.py tests/scantool-minix/minix-relocate.img.processed.out `expr 512 \* 102400` tests/scantool-minix/minix-spock.png 250 300
util/draw_scan.py tests/scantool-vfat/vfat-relocate.img.processed.out `expr 1024 \* 102400` tests/scantool-vfat/vfat-spock.png 250 300
util/draw_scan.py tests/scantool-vfat/vfat-reloc-greenspan.img.processed.out `expr 1024 \* 102400` tests/scantool-vfat/vfat-greenspan.png 250 300
util/draw_scan.py tests/scantool-vfat/vfat-reloc-greenspan.img.processed.out `expr 1024 \* 102400` tests/scantool-vfat/vfat-greenspan.eps 250 300
util/draw_scan.py tests/scantool-reiserfs/reiserfs-relocate.img.processed.out `expr 1024 \* 102400` tests/scantool-reiserfs/reiserfs-spock.png 250 300
util/draw_scan.py tests/scantool-reiserfs/reiserfs-relocate-problem.img.processed.out `expr 1024 \* 102400` tests/scantool-reiserfs/reiserfs-problem.png 250 300