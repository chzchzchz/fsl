OBJS=	parser.o	\
	lex.yy.o	\
	main.o		\
	type.o		\
	eval.o

LLVM_FLAGS:=$(shell llvm-config --cxxflags)
CFLAGS= -g $(LLVM_FLAGS) 

all: lang


 
lang:  $(OBJS)
	g++ $(CFLAGS)  $^ -o $@	


parser.hh: parser.cc

parser.cc: parser.y
	bison -d -o $@ $^

lex.yy.cc: lang.lex parser.hh
	flex -o $@ $^

.cc.o:
	g++ $(CFLAGS) -c $< -o $@


clean: 
	rm -f *.o lang lex.yy.cc y.tab.h parser.cc parser.hh
