ifndef CFLAGS
    $(error No CFLAGS. Run from top level.)
endif

ifndef OBJDIR
	$(error No OBJDIR. Run from top level.)
endif

ifndef BINDIR
	$(error No OBJDIR. Run from top level.)
endif

LANGOBJS=	lex.yy.o		\
		parser.o		\
		main.o			\
		type.o			\
		eval.o			\
		symtab.o		\
		expr.o			\
		cond.o			\
		fcall.o			\
		func.o			\
		evalctx.o		\
		annotation.o		\
		symtab_thunkbuilder.o	\
		code_builder.o		\
		table_gen.o		\
		thunk_type.o		\
		thunk_fieldoffset.o	\
		thunk_elems.o		\
		thunk_field.o		\
		thunk_fieldsize.o	\
		thunk_size.o		\
		thunk_func.o		\
		thunk_params.o			\
		thunk_fieldoffset_cond.o	\
		thunk_fieldfunc.o	\
		preamble.o		\
		points.o		\
		asserts.o		\
		runtime_interface.o	\
		args.o			\
		virt.o			\
		writepkt.o		\
		instance_iter.o		\
		typeclosure.o		\
		type_stack.o		\
		reloc.o			\
		varscope.o		\
		wpkt_inst.o		\
		wpkt_struct.o		\
		wpkt_call.o		\
		memotab.o

USER_RTOBJS= runtime.o	\
	type_info.o 	\
	virt.o		\
	io.o		\
	entry.o		\
	rt-pread/io.o		\
	rt-pread/cache.o	\
	rt-mmap/io.o		\
	io_mmap.o		\
	io_pread.o		\
	max.o			\
	debug.o			\
	debug_libc.o		\
	info_libc.o		\
	hits.o			\
	log.o			\
	type_print.o		\
	usemap.o		\
	bitmap.o		\
	scan.o			\
	choice.o		\
	writepkt.o		\
	lookup.o		\
	alloc_libc.o

KERN_RTOBJS=	runtime.o	\
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

KERN_TOOLOBJS = smushtool.o scattertool.o defragtool.o

TOOLOBJS=	scantool.o browser.o modify.o fragtool.o relocate.o defragtool.o \
		scattertool.o smushtool.o fuse-browser.o fuse_node.o thrashcopy.o

RTOBJSOUT := $(USER_RTOBJS:%=$(OBJDIR)/runtime/%)
KLEERTOBJSOUT := $(USER_RTOBJS:%=$(OBJDIR)/klee/runtime/%)
RTKOBJSOUT := $(KERN_RTOBJS:%=$(OBJDIR)/kernel/%)
TOOLOBJSOUT := $(TOOLOBJS:%=$(OBJDIR)/tool/%)
KLEETOOLOBJSOUT := $(TOOLOBJS:%=$(OBJDIR)/klee/tool/%)
KTOOLOBJSOUT := $(KERN_TOOLOBJS:%=$(OBJDIR)/kernel/tool/%)
LANGOBJSOUT := $(LANGOBJS:%=$(OBJDIR)/compiler/%)

DEFINE_FLAGS=-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
LLVM_FLAGS_ORIGINAL:=$(shell llvm-config --cxxflags --ldflags --libs core)
LLVM_FLAGS:=$(shell echo "$(LLVM_FLAGS_ORIGINAL)" |  sed "s/-Woverloaded-virtual//;s/-fPIC//;s/-DNDEBUG//g") -Wall
LLVM_LD:=$(shell llvm-config --cxxflags --ldflags --libs core)
COMP_CFLAGS= $(CFLAGS) $(LLVM_FLAGS)  $(DEFINE_FLAGS) -Wall
RTCFLAGS= $(CFLAGS) $(DEFINE_FLAGS) -Wall
RTKCFLAGS= $(CFLAGS) $(DEFINE_FLAGS) -mcmodel=kernel -mno-red-zone -Wall  -D__KERNEL__
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
	llvm-gcc -DUSE_KLEE $(RTCFLAGS) --emit-llvm -Iruntime/ -c $< -o $@
$(OBJDIR)/klee/runtime/io_pread.o: 	\
	$(OBJDIR)/klee/runtime/rt-pread/io.o \
	$(OBJDIR)/klee/runtime/rt-pread/cache.o
	llvm-ld -r -o $@ $^
$(OBJDIR)/klee/runtime/io_mmap.o: $(OBJDIR)/klee/runtime/rt-mmap/io.o
	cp $^ $@
$(OBJDIR)/klee/tool/%.o: tool/%.c
	llvm-gcc -DUSE_KLEE $(RTCFLAGS) --emit-llvm -Iruntime/ -c $< -o $@

# LANGUAGE
compiler/parser.hh: compiler/parser.cc

compiler/parser.cc: compiler/parser.y
	bison -d -o $@ $^

compiler/lex.yy.cc: compiler/lang.lex
	flex -o $@ $^

$(BINDIR)/lang:  $(LANGOBJSOUT) $(RTOBJSOUT) $(TOOLOBJSOUT)
	g++ $(DEBUG_FLAG) -O3 $(LANGOBJSOUT) $(LLVM_FLAGS) -o $@
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
