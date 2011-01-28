FSNAMES= ext2 vfat iso9660 reiserfs xfs
NUM_JOBS=6
MAKECMD=make -j$(NUM_JOBS)
#CFLAGS=-O3 -DFSL_RELEASE
OPT_FLAGS=-O3
LLC_FLAGS=-O3
CFLAGS=-g -O3 -DFSL_LITTLE_ENDIAN
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
export FSNAMES
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

TEST_EXTRA_MMAP=$(FSNAMES:%=tests-extra-mmap-%)
tests-extra-mmap: $(TEST_EXTRA_MMAP)
tests-extra-mmap-%:
	TOOL_RT=mmap TEST_CONFIG="EXTRA" USE_STATS="YES" TEST_FS=`echo $@ | cut -f4 -d-`  tests/do_all_tests.sh



TEST_EXTRA=$(FSNAMES:%=tests-extra-std-%)
tests-extra: tests-extra-std
tests-extra-std: $(TEST_EXTRA)
tests-extra-std-%:
	TEST_CONFIG="EXTRA" USE_STATS="YES" TEST_FS=`echo $@ | cut -f4 -d-` tests/do_all_tests.sh

tests-depth-stack:
	tests/tests_depth.sh

TESTS_EXTRA_STACK=$(FSNAMES:%=tests-extra-stack-%)
tests-extra-stack: $(TESTS_EXTRA_STACK)
tests-extra-stack-%:
	TEST_CONFIG="STACK" TEST_FS=`echo $@ | cut -f4 -d-` tests/do_all_tests.sh


TESTS_EXTRA_MEM=$(FSNAMES:%=tests-extra-mem-%)
tests-extra-mem-stack: $(TESTS_EXTRA_MEM)
tests-extra-mem-%:
	TEST_CONFIG="MEM" TEST_FS=`echo $@ | cut -f4 -d-` tests/do_all_tests.sh

TESTS_EXTRA_OPROF=$(FSNAMES:%=tests-extra-oprof-%)
tests-extra-oprof: $(TESTS_EXTRA_OPROF)
tests-extra-oprof-%:
	TEST_CONFIG="EXTRA" USE_OPROF="YES" TEST_FS=`echo $@ | cut -f4 -d-`  tests/do_all_tests.sh

tests-clean:
	rm -f tests/scantool-*/*
	rm -f tests/relocate-*/*
	rm -f tests/defragtool-*/*


libs-clean:
	make -C lib clean
