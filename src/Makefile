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
		reloc.o			\
		varscope.o		\
		wpkt_inst.o		\
		wpkt_struct.o		\
		wpkt_call.o		\
		memotab.o

RTOBJS= runtime.o	\
	type_info.o 	\
	virt.o		\
	io.o		\
	max.o		\
	debug.o		\
	hits.o		\
	log.o		\
	cache.o		\
	type_print.o	\
	usemap.o	\
	bitmap.o	\
	scan.o		\
	choice.o	\
	writepkt.o

TOOLOBJS=	scantool.o browser.o modify.o fragtool.o relocate.o defragtool.o \
		scattertool.o smushtool.o fuse-browser.o
RTOBJSOUT := $(RTOBJS:%=$(OBJDIR)/runtime/%)
TOOLOBJSOUT := $(TOOLOBJS:%=$(OBJDIR)/tool/%)
LANGOBJSOUT := $(LANGOBJS:%=$(OBJDIR)/compiler/%)

DEFINE_FLAGS=-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
LLVM_FLAGS_ORIGINAL:=$(shell llvm-config --cxxflags --ldflags --libs core) 
LLVM_FLAGS:=$(shell echo "$(LLVM_FLAGS_ORIGINAL)" |  sed "s/-Woverloaded-virtual//;s/-fPIC//;s/-DNDEBUG//g") -Wall
LLVM_LD:=$(shell llvm-config --cxxflags --ldflags --libs core)
COMP_CFLAGS= $(CFLAGS) $(LLVM_FLAGS)  $(DEFINE_FLAGS) -Wall
RTCFLAGS= $(CFLAGS) $(DEFINE_FLAGS) -Wall
export CFLAGS
export OBJDIR

all: $(BINDIR)/lang

scantool: $(BINDIR)/lang
	cd tool && make all
 
$(BINDIR)/lang:  $(LANGOBJSOUT) $(RTOBJSOUT) $(TOOLOBJSOUT)
	g++ $(DEBUG_FLAG) -O3 $(LANGOBJSOUT) $(LLVM_FLAGS) -o $@
	cd tool && make clean	# outdated now..

compiler/parser.hh: compiler/parser.cc

compiler/parser.cc: compiler/parser.y
	bison -d -o $@ $^

compiler/lex.yy.cc: compiler/lang.lex
	flex -o $@ $^

$(OBJDIR)/tool/%.o: tool/%.c
	gcc $(RTCFLAGS) -Iruntime/ -c $< -o $@

$(OBJDIR)/runtime/%.o: runtime/%.c
	gcc $(RTCFLAGS) -c $< -o $@

$(OBJDIR)/compiler/%.o: compiler/%.cc
	g++ $(COMP_CFLAGS) -c $< $(LLVM_FLAGS) -o $@

clean: 
	rm -f  compiler/lex.yy.cc compiler/y.tab.h compiler/parser.cc compiler/parser.hh  fsl.table.c *.ll $(OBJDIR)/*.o $(OBJDIR)/fs/* $(OBJDIR)/runtime/*.o $(BINDIR)/* $(OBJDIR)/compiler/*.o
	cd tool && make clean
