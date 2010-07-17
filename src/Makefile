OBJS=	parser.o		\
	lex.yy.o		\
	main.o			\
	type.o			\
	eval.o			\
	symtab.o		\
	expr.o			\
	cond.o			\
	func.o			\
	phys_type.o		\
	evalctx.o		\
	funcevalctx.o		\
	thunk.o			\
	symtab_thunkbuilder.o	\
	symtab_builder.o	

RTOBJS= runtime.o

DEFINE_FLAGS=-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
LLVM_FLAGS_ORIGINAL:=$(shell llvm-config --cxxflags --ldflags --libs core)
LLVM_FLAGS:=$(shell echo "$(LLVM_FLAGS_ORIGINAL)" |  sed "s/-Woverloaded-virtual//")
LLVM_LD:=$(shell llvm-config --cxxflags --ldflags --libs core)
DEBUG_FLAG=-g
CFLAGS= $(DEBUG_FLAG) $(LLVM_FLAGS)  $(DEFINE_FLAGS) 

all: lang


 
lang:  $(OBJS) $(RTOBJS)
	g++ $(DEBUG_FLAG) -O3 $(OBJS) $(LLVM_FLAGS) -o $@	


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
	rm -f *.o lang lex.yy.cc y.tab.h parser.cc parser.hh
