
all: lang
 
lang:  parser.o lex.yy.o main.o
	g++  $^ -o $@	


parser.hh: parser.cc

parser.cc: parser.y
	bison -d -o $@ $^

lex.yy.cc: lang.lex parser.hh
	flex -o $@ $^

.cc.o:
	g++ -c $< -o $@


clean: 
	rm -f *.o lang lex.yy.cc y.tab.h parser.cc parser.hh
