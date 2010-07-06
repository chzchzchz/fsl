OBJS=	parser.o	\
	lex.yy.o	\
	main.o		\
	type.o		\
	eval.o		\
	symtab.o	\
	expr.o		\
	cond.o		\
	func.o		\
	phys_type.o	\
	evalctx.o	\
	funcevalctx.o	\
	thunk.o

LLVM_FLAGS_ORIGINAL:=$(shell llvm-config --cxxflags --ldflags --libs core)
LLVM_FLAGS:=$(shell echo "$(LLVM_FLAGS_ORIGINAL)" |  sed "s/-Woverloaded-virtual//")
LLVM_LD:=$(shell llvm-config --cxxflags --ldflags --libs core)
CFLAGS= -g $(LLVM_FLAGS) 

all: lang


 
lang:  $(OBJS)
	g++ -g -O3 $^ $(LLVM_FLAGS) -o $@	


parser.hh: parser.cc

parser.cc: parser.y
	bison -d -o $@ $^

lex.yy.cc: lang.lex parser.hh
	flex -o $@ $^

.cc.o:
	g++ -g -O3 -c $< $(LLVM_FLAGS) -o $@


clean: 
	rm -f *.o lang lex.yy.cc y.tab.h parser.cc parser.hh
