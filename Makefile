FSNAMES= ext2 vfat iso9660 reiserfs xfs minix
NUM_JOBS=6
#LINUX_SRCDIR=/usr/src/linux/
LINUX_SRCDIR=/home/chz/src/research/FSL_VM_LINUX/
MAKECMD=make -j$(NUM_JOBS)
CFLAGS=-O3 -DFSL_RELEASE -DNDEBUG -DFSL_LITTLE_ENDIAN
OPT_FLAGS=-O3
LLC_FLAGS=-O3
#CFLAGS=-g -O3 -DFSL_LITTLE_ENDIAN -fno-common
#CFLAGS=-g
export CFLAGS
OBJDIR=$(shell pwd)/obj/
BINDIR=$(shell pwd)/bin/
FSSRCDIR=$(shell pwd)/fs/
KLEEBINDIR=/home/chz/src/klee/Release/bin/
export OBJDIR
export BINDIR
export FSSRCDIR
export OPT_FLAGS
export LLC_FLAGS
export FSNAMES
export LINUX_SRCDIR
all: code tools tests draw

clean: code-clean tests-clean
	rm -f bin/*-* bin/lang bin/*/*

draw: draw-hits draw-scans draw-relocs

draw-hits:
	util/draw_all_hits.sh

draw-scans:
	util/draw_all_scans.sh

draw-relocs:
	util/draw_all_relocs.sh

klee:
	cd src && $(MAKECMD) klee && cd ..

code:
	cd src && $(MAKECMD) && cd ..

tools: code klee
	cd src/tool && $(MAKECMD) && cd ../..

kern: tools
	cd src && $(MAKECMD) kern && cd ..

libs:
	make -C lib all

code-clean:
	cd src && make clean && cd ..

tests: code tools
	tests/do_all_tests.sh

paper: paper-gen paper-tests paper-draw
paper-gen: tests
paper-tests: tests-extra tests-extra-oprof tests-extra-stack tests-depth-stack
paper-draw: draw

KLEEFLAGS=-max-instruction-time=30.  --max-memory-inhibit=false   --use-random-path -max-static-fork-pct=30 -max-static-solve-pct=30 --max-static-cpfork-pct=30  --disable-inlining --use-interleaved-covnew-NURS  --use-batching-search --batch-instructions 1000   -weight-type=covnew  --only-output-states-covering-new  -use-cache -use-cex-cache --optimize -libc=uclibc -posix-runtime -init-env
KLEEENV=-sym-args 2 2 64 -sym-files  1 4194304

TESTS_KLEE=$(FSNAMES:%=tests-klee-%)
tests-klee: $(TESTS_KLEE)
tests-klee-%:
	mkdir -p tests/klee-`echo $@ | cut -f3 -d- `
	cp $(BINDIR)/klee/scantool-`echo $@ | cut -f3 -d- `.bc \
		tests/klee-`echo $@ | cut -f3 -d- `/
	cd tests/klee-`echo $@ | cut -f3 -d-` && \
	$(KLEEBINDIR)/klee $(KLEEFLAGS) ./scantool-`echo $@ | cut -f3 -d-`.bc \
		$(KLEEENV)

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

TEST_EXTRA=$(FSNAMES:%=tests-extra-std-%)
tests-extra-diskstat: $(TEST_EXTRA)
tests-extra-diskstat-%:
	TEST_CONFIG="EXTRA" USE_SYNC="YES" USE_STATS="YES" TEST_FS=`echo $@ | cut -f4 -d-` tests/do_all_tests.sh

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
