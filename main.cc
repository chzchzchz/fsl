#include <iostream>
#include "AST.h"

extern int yyparse();

extern Scope* global_scope;

using namespace std;

int main(int argc, char *argv[])
{
	yyparse();
	cout << "OK" << endl;
	cout << global_scope->getStmts() << endl;
	return 0;
}
