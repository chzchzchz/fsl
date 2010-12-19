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

tests-extra: tests-extra-ext2 tests-extra-vfat

tests-extra-oprof: tests-extra-ext2-oprof tests-extra-vfat-oprof

tests-extra-ext2-oprof:
	TEST_CONFIG="EXTRA" OPROFILE_FLAG="OK" TEST_FS="ext2"  tests/do_all_tests.sh
tests-extra-vfat-oprof:
	TEST_CONFIG="EXTRA" OPROFILE_FLAG="OK" TEST_FS="vfat"  tests/do_all_tests.sh

tests-extra-ext2:
	TEST_CONFIG="EXTRA" TEST_FS="ext2"  tests/do_all_tests.sh

tests-extra-vfat:
	TEST_CONFIG="EXTRA" TEST_FS="vfat" tests/do_all_tests.sh

tests-clean:
	rm -f tests/scantool-*/*

libs-clean:
	make -C lib clean
