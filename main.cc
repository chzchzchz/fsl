#include <iostream>
#include "AST.h"
#include "type.h"

extern int yyparse();

extern Scope* global_scope;

using namespace std;

ostream& operator<<(ostream& in, const Type& t)
{
	t.print(in);
	return in;
}

ostream& operator<<(ostream& in, const GlobalStmt& gs)
{
	gs.print(in);
	return in;
}

int main(int argc, char *argv[])
{
	Scope::iterator	it;

	yyparse();

	for (it = global_scope->begin(); it != global_scope->end(); it++) {
		GlobalStmt	*gs = *it;
		cout << *gs << endl << endl;
	}

	return 0;
}
