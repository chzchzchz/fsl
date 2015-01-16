ifndef CFLAGS
    $(error No CFLAGS. Run from top level.)
endif

ifndef OBJDIR
	$(error No OBJDIR. Run from top level.)
endif

ifndef BINDIR
	$(error No OBJDIR. Run from top level.)
endif

LANGOBJS=$(patsubst %.cc,%.o,$(wildcard compiler/*.cc))
USER_RTOBJS=$(patsubst %.c,%.o,$(wildcard runtime/*.c) $(wildcard runtime/*/*.c))
KERN_RTOBJS0=	runtime.o	\
		bridge.o	\
		type_info.o 	\
		type_print.o	\
		virt.o		\
		io.o		\
		max.o		\
		debug.o		\
		log.o		\
		bitmap.o	\
		scan.o		\
		choice.o	\
		writepkt.o	\
		lookup.o	\
KERN_RTOBJS=$(KERN_RTOBJS0:%=runtime/%)

KERN_TOOLOBJS0 = smushtool.o scattertool.o defragtool.o
KERN_TOOLOBJS=$(KERN_TOOLOBJS0:%=tool/%)
TOOLOBJS=$(patsubst %.c,%.o,$(wildcard tool/*.c))


RTOBJSOUT := $(USER_RTOBJS:%=$(OBJDIR)/%)
KLEERTOBJSOUT := $(USER_RTOBJS:%=$(OBJDIR)/klee/%)
RTKOBJSOUT := $(KERN_RTOBJS:%=$(OBJDIR)/kernel/%)
TOOLOBJSOUT := $(TOOLOBJS:%=$(OBJDIR)/%)
KLEETOOLOBJSOUT := $(TOOLOBJS:%=$(OBJDIR)/klee/%)
KTOOLOBJSOUT := $(KERN_TOOLOBJS:%=$(OBJDIR)/kernel/%)
LANGOBJSOUT := $(LANGOBJS:%=$(OBJDIR)/%)

DEFINE_FLAGS=-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
LLVM_FLAGS_ORIGINAL:=$(shell llvm-config --cxxflags --ldflags --libs core)
LLVM_FLAGS:=$(shell echo "$(LLVM_FLAGS_ORIGINAL)" |  sed "s/-Woverloaded-virtual//;s/-fPIC//;s/-DNDEBUG//g") -Wall
LLVM_LD:=$(shell llvm-config --libs core)  $(shell llvm-config --ldflags) -lpthread -lrt -lncurses -ldl
COMP_CFLAGS= $(CFLAGS) $(LLVM_FLAGS)  $(DEFINE_FLAGS) -Wall
RTCFLAGS= $(CFLAGS) $(DEFINE_FLAGS) -Wall
RTKCFLAGS= $(CFLAGS) $(DEFINE_FLAGS) 	\
	-I$(LINUX_SRCDIR)/include/ 	\
	-I$(LINUX_SRCDIR)/arch/x86/include/ 	\
	-mcmodel=kernel -mno-red-zone -Wall  -D__KERNEL__
export CFLAGS
export OBJDIR

all: $(BINDIR)/lang $(KTOOLOBJSOUT)

scantool: $(BINDIR)/lang
	cd tool && $(MAKECMD)  all

kern: $(RTKOBJSOUT)
ifdef LINUX_SRCDIR
	cd kernel; make; cd ..
endif

#KLEE
klee: $(KLEERTOBJSOUT) $(KLEETOOLOBJSOUT)

$(OBJDIR)/klee/runtime/%.o: runtime/%.c
	clang -DUSE_KLEE $(RTCFLAGS) -emit-llvm -Iruntime/ -c $< -o $@
$(OBJDIR)/klee/runtime/io_pread.o: 	\
	$(OBJDIR)/klee/runtime/rt-pread/io.o \
	$(OBJDIR)/klee/runtime/rt-pread/cache.o
	llvm-link -o $@ $^
$(OBJDIR)/klee/runtime/io_mmap.o: $(OBJDIR)/klee/runtime/rt-mmap/io.o
	cp $^ $@
$(OBJDIR)/klee/tool/%.o: tool/%.c
	clang -DUSE_KLEE $(RTCFLAGS) -emit-llvm -Iruntime/ -c $< -o $@

# LANGUAGE
compiler/parser.hh: compiler/parser.cc

compiler/parser.cc: compiler/parser.y
	bison -d -o $@ $^

compiler/lex.yy.cc: compiler/lang.lex
	flex -o $@ $^

$(BINDIR)/lang:  $(LANGOBJSOUT) $(RTOBJSOUT) $(TOOLOBJSOUT)
	g++ $(DEBUG_FLAG) -O3 $(LANGOBJSOUT) $(LLVM_LD) $(LLVM_FLAGS) -o $@
	cd tool && make clean	# outdated now..

# TOOLS
$(OBJDIR)/tool/%.o: tool/%.c
	gcc $(RTCFLAGS) -Iruntime/ -c $< -o $@

$(OBJDIR)/runtime/%.o: runtime/%.c
	gcc $(RTCFLAGS) -Iruntime/ -c $< -o $@

$(OBJDIR)/kernel/%.o: runtime/%.c
	gcc $(RTKCFLAGS) -Iruntime/ -c $< -o $@

$(OBJDIR)/kernel/tool/%.o: tool/%.c
	gcc $(RTKCFLAGS) -Iruntime/ -c $< -o $@

$(OBJDIR)/compiler/%.o: compiler/%.cc
	g++ $(COMP_CFLAGS) -c $< $(LLVM_FLAGS) -o $@

$(OBJDIR)/runtime/io_pread.o: 		\
	$(OBJDIR)/runtime/rt-pread/io.o \
	$(OBJDIR)/runtime/rt-pread/cache.o
	ld -r -o $@ $^

$(OBJDIR)/runtime/io_mmap.o: $(OBJDIR)/runtime/rt-mmap/io.o
	cp $^ $@

clean:
	rm -f  compiler/lex.yy.cc compiler/y.tab.h compiler/parser.cc compiler/parser.hh  fsl.table.c *.ll \
	$(RTOBJSOUT) \
	$(LANGOBJSOUT) \
	$(KTOOLOBJSOUT)	\
	$(RTKOBJSOUT)	\
	$(TOOLOBJSOUT)	\
	$(KLEERTOBJSOUT) \
	$(KLEETOOLOBJSOUT) \
	$(OBJDIR)/fs/*
	cd tool && make clean
	cd kernel && make clean
