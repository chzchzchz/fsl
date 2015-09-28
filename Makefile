FILESYSTEMS :=	btrfs		\
		ext2 		\
		hfsplus		\
		iso9660		\
		minix		\
		nilfs2 		\
		pcpartfs	\
		reiserfs	\
		testfs 		\
		vfat 		\
		vmdk		\
		xfs
FSNAMES= $(FILESYSTEMS)


space :=
space +=

NUM_JOBS=6
#LINUX_SRCDIR=/usr/src/linux/
ifndef LINUX_SRCDIR
	LINUX_SRCDIR:=/home/chz/src/research/FSL_VM_LINUX/
endif
MAKECMD=make -j$(NUM_JOBS)
#CFLAGS= -g -O3 -DFSL_RELEASE -DNDEBUG -DFSL_LITTLE_ENDIAN -fno-common
#CFLAGS=-g  -DFSL_LITTLE_ENDIAN -fno-common
CFLAGS= -g -O3 -DFSL_RELEASE -DFSL_LITTLE_ENDIAN -fno-common
OPT_FLAGS=-O3
LLC_FLAGS=-O3
LLVMCC=clang
LLVMCXX=clang++
CC=gcc
CXX=g++
export CFLAGS
OBJDIR=$(shell pwd)/obj
BINDIR=$(shell pwd)/bin
IMGDIR=$(shell pwd)/img
FSSRCDIR=$(shell pwd)/fs
KLEEBINDIR=/home/chz/klee/Release/bin/
export OBJDIR
export BINDIR
export FSSRCDIR
export OPT_FLAGS
export LLC_FLAGS
export FSNAMES
export LINUX_SRCDIR

.PHONY: all
all: code tools

include src/Makefile
include Makefile.tests
include Makefile.img


.PHONY: clean
clean: code-clean tests-clean clean-root
	rm -f bin/*-* bin/lang bin/*/*

clean-root:
	rm -f cur_test*
	rm -f fsck*
	rm -f fuse*
	rm -f err pin.log pintool.log run_test.out tests.log

draw: draw-hits draw-scans draw-relocs

OBJDIRS=	compiler fs tool			\
		runtime/rt-mmap runtime/rt-pread	\
		kernel/tool				\
		klee/tool klee/runtime/rt-mmap klee/runtime/rt-pread
OBJDIRS:=$(OBJDIRS:%=$(OBJDIR)/%)

$(OBJDIR):
	mkdir -p $(OBJDIRS)

.PHONY: libs
libs:
	make -C lib all

.PHONY: libs-clean
libs-clean:
	make -C lib clean

.PHONY: imgs
imgs: imgs-all

.PHONY: scan-build
scan-build:
	mkdir -p scan-out
	scan-build --use-cc="$(CC)" --use-c++="$(CXX)" \
		`clang -cc1 -analyzer-checker-help | awk ' { print "-enable-checker="$1 } ' | grep '\.' | grep -v debug ` \
		-o `pwd`/scan-out $(MAKECMD) all
