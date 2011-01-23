NUM_JOBS=6
MAKECMD=make -j$(NUM_JOBS)
#CFLAGS=-O3 -DFSL_RELEASE
OPT_FLAGS=-O3
LLC_FLAGS=-O3
CFLAGS=-g -O3
#CFLAGS=-g
export CFLAGS
OBJDIR=$(shell pwd)/obj/
BINDIR=$(shell pwd)/bin/
FSSRCDIR=$(shell pwd)/fs/
export OBJDIR
export BINDIR
export FSSRCDIR
export OPT_FLAGS
export LLC_FLAGS
all: code tools tests draw

clean: code-clean tests-clean

draw: draw-hits draw-scans draw-relocs

draw-hits:
	util/draw_all_hits.sh

draw-scans:
	util/draw_all_scans.sh

draw-relocs:
	util/draw_all_relocs.sh

code:
	cd src && $(MAKECMD) && cd ..

tools: code
	cd src/tool && $(MAKECMD) && cd ../..

libs:
	make -C lib all

code-clean:
	cd src && make clean && cd ..

tests: code tools
	tests/do_all_tests.sh

tests-mmap:
	TOOL_RT=mmap tests/do_all_tests.sh

tests-linux-4gb: tests-linux-4gb-vfat tests-linux-4gb-ext2 tests-linux-4gb-reiserfs
tests-linux-4gb-vfat:
	FILESYSTEM=vfat USE_STATS="YES" tests/linux-4gb.sh
tests-linux-4gb-ext2:
	FILESYSTEM=ext2 USE_STATS="YES" tests/linux-4gb.sh
tests-linux-4gb-reiserfs:
	FILESYSTEM=reiserfs USE_STATS="YES" tests/linux-4gb.sh

tests-mmap-extra: 	tests-mmap-extra-ext2 tests-mmap-extra-vfat \
			tests-mmap-extra-reiserfs tests-mmap-extra-iso9660

tests-mmap-extra-reiserfs:
	TOOL_RT=mmap TEST_CONFIG="EXTRA" USE_STATS="YES" TEST_FS="reiserfs"  tests/do_all_tests.sh
tests-mmap-extra-ext2:
	TOOL_RT=mmap TEST_CONFIG="EXTRA" USE_STATS="YES" TEST_FS="ext2"  tests/do_all_tests.sh
tests-mmap-extra-vfat:
	TOOL_RT=mmap TEST_CONFIG="EXTRA" USE_STATS="YES" TEST_FS="vfat" tests/do_all_tests.sh
tests-mmap-extra-iso9660:
	TOOL_RT=mmap TEST_CONFIG="EXTRA" USE_STATS="YES" TEST_FS="iso9660" tests/do_all_tests.sh

tests-extra: tests-extra-ext2 tests-extra-vfat tests-extra-reiserfs tests-extra-iso9660
tests-extra-reiserfs:
	TEST_CONFIG="EXTRA" USE_STATS="YES" TEST_FS="reiserfs"  tests/do_all_tests.sh
tests-extra-ext2:
	TEST_CONFIG="EXTRA" USE_STATS="YES" TEST_FS="ext2"  tests/do_all_tests.sh
tests-extra-vfat:
	TEST_CONFIG="EXTRA" USE_STATS="YES" TEST_FS="vfat" tests/do_all_tests.sh
tests-extra-iso9660:
	TEST_CONFIG="EXTRA" USE_STATS="YES" TEST_FS="iso9660" tests/do_all_tests.sh

tests-depth-stack:
	tests/tests_depth.sh

tests-extra-stack: tests-extra-stack-vfat tests-extra-stack-ext2 tests-extra-stack-reiserfs tests-extra-stack-iso9660
tests-extra-stack-vfat:
	TEST_CONFIG="STACK" TEST_FS="vfat" tests/do_all_tests.sh
tests-extra-stack-ext2:
	TEST_CONFIG="STACK" TEST_FS="ext2" tests/do_all_tests.sh
tests-extra-stack-reiserfs:
	TEST_CONFIG="STACK" TEST_FS="reiserfs" tests/do_all_tests.sh
tests-extra-stack-iso9660:
	TEST_CONFIG="STACK" TEST_FS="iso9660" tests/do_all_tests.sh


tests-extra-mem: tests-extra-mem-vfat tests-extra-mem-ext2 tests-extra-mem-reiserfs tests-extra-mem-iso9660
tests-extra-mem-vfat:
	TEST_CONFIG="MEM" TEST_FS="vfat" tests/do_all_tests.sh
tests-extra-mem-ext2:
	TEST_CONFIG="MEM" TEST_FS="ext2" tests/do_all_tests.sh
tests-extra-mem-reiserfs:
	TEST_CONFIG="MEM" TEST_FS="reiserfs" tests/do_all_tests.sh
tests-extra-mem-iso9660:
	TEST_CONFIG="MEM" TEST_FS="iso9660" tests/do_all_tests.sh

tests-extra-oprof: tests-extra-ext2-oprof tests-extra-vfat-oprof tests-extra-reiserfs-oprof tests-extra-iso9660-oprof
tests-extra-ext2-oprof:
	TEST_CONFIG="EXTRA" USE_OPROF="YES" TEST_FS="ext2"  tests/do_all_tests.sh
tests-extra-vfat-oprof:
	TEST_CONFIG="EXTRA" USE_OPROF="YES" TEST_FS="vfat"  tests/do_all_tests.sh
tests-extra-reiserfs-oprof:
	TEST_CONFIG="EXTRA" USE_OPROF="YES" TEST_FS="reiserfs"  tests/do_all_tests.sh
tests-extra-iso9660-oprof:
	TEST_CONFIG="EXTRA" USE_OPROF="YES" TEST_FS="iso9660"  tests/do_all_tests.sh

tests-clean:
	rm -f tests/scantool-*/*
	rm -f tests/relocate-*/*
	rm -f tests/defragtool-*/*


libs-clean:
	make -C lib clean
