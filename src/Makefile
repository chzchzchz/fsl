OBJS=	parser.o		\
	lex.yy.o		\
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
	thunk_fieldfunc.o		\
	preamble.o			\
	points.o			\
	asserts.o			\
	runtime_interface.o		\
	args.o				\
	virt.o

RTOBJS= runtime/runtime.o runtime/type_info.o runtime/virt.o
TOOLOBJS= runtime/scantool.o runtime/browser.o

DEFINE_FLAGS=-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
LLVM_FLAGS_ORIGINAL:=$(shell llvm-config --cxxflags --ldflags --libs core) 
LLVM_FLAGS:=$(shell echo "$(LLVM_FLAGS_ORIGINAL)" |  sed "s/-Woverloaded-virtual//;s/-fPIC//") -Wall
LLVM_LD:=$(shell llvm-config --cxxflags --ldflags --libs core)
DEBUG_FLAG=-g
CFLAGS= $(DEBUG_FLAG) $(LLVM_FLAGS)  $(DEFINE_FLAGS) -Wall

all: lang

scantool: lang
	cd tool && make all
 
lang:  $(OBJS) $(RTOBJS) $(TOOLOBJS)
	g++ $(DEBUG_FLAG) -O3 $(OBJS) $(LLVM_FLAGS) -o $@
	cd tool && make clean	# outdated now..

parser.hh: parser.cc

parser.cc: parser.y
	bison -d -o $@ $^

lex.yy.cc: lang.lex parser.hh
	flex -o $@ $^

.cc.o:
	g++ $(DEBUG_FLAG) -O3 -c $< $(LLVM_FLAGS) -o $@

.c.o:
	gcc $(DEFINE_FLAGS) $(DEBUG_FLAG) -O3 -c $< -o $@

clean: 
	rm -f  lang lex.yy.cc y.tab.h parser.cc parser.hh $(OBJS) $(RTOBJS) $(TOOLOBJS)  fsl.table.c *.ll
	cd tool && make clean
