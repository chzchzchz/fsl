NUM_JOBS=6
MAKECMD=make -j$(NUM_JOBS)
CFLAGS=-g -O3
#CFLAGS=-g
export CFLAGS

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

tests-extra: tests-extra-ext2 tests-extra-vfat tests-extra-reiserfs
tests-extra-reiserfs:
	TEST_CONFIG="EXTRA" USE_STATS="YES" TEST_FS="reiserfs"  tests/do_all_tests.sh
tests-extra-ext2:
	TEST_CONFIG="EXTRA" USE_STATS="YES" TEST_FS="ext2"  tests/do_all_tests.sh
tests-extra-vfat:
	TEST_CONFIG="EXTRA" USE_STATS="YES" TEST_FS="vfat" tests/do_all_tests.sh


tests-extra-stack: tests-extra-stack-vfat tests-extra-stack-ext2 tests-extra-stack-iso9660
tests-extra-stack-vfat:
	TEST_CONFIG="STACK" TEST_FS="vfat" tests/do_all_tests.sh
tests-extra-stack-ext2:
	TEST_CONFIG="STACK" TEST_FS="ext2" tests/do_all_tests.sh
tests-extra-stack-iso9660:
	TEST_CONFIG="STACK" TEST_FS="iso9660" tests/do_all_tests.sh


tests-extra-mem: tests-extra-mem-vfat tests-extra-mem-ext
tests-extra-mem-vfat:
	TEST_CONFIG="MEM" TEST_FS="vfat" tests/do_all_tests.sh
tests-extra-mem-ext2:
	TEST_CONFIG="MEM" TEST_FS="ext2" tests/do_all_tests.sh

tests-extra-oprof: tests-extra-ext2-oprof tests-extra-vfat-oprof
tests-extra-ext2-oprof:
	TEST_CONFIG="EXTRA" USE_OPROF="YES" TEST_FS="ext2"  tests/do_all_tests.sh
tests-extra-vfat-oprof:
	TEST_CONFIG="EXTRA" USE_OPROF="YES" TEST_FS="vfat"  tests/do_all_tests.sh

tests-clean:
	rm -f tests/scantool-*/*
	rm -f tests/relocate-*/*
	rm -f tests/defragtool-*/*


libs-clean:
	make -C lib clean
