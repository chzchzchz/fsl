#include <iostream>
#include <string>
#include <map>
#include "AST.h"
#include "type.h"
#include "phys_type.h"

extern int yyparse();

extern GlobalBlock* global_scope;

using namespace std;

type_map	types;

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

static void load_primitive_types(void)
{
	PhysicalType	*pt[] = {
		new U8(),
		new U16(),
		new U32(),
		new U64(),
		new I16(),
		new I32(),
		new U64(),
		new U12()};
	
	for (unsigned int i = 0; i < (sizeof(pt)/sizeof(PhysicalType*)); i++) {
		types[pt[i]->getName()] = pt[i];
	}
}

#if 0
static void load_user_types(void)
{
	GlobalBlock::iterator	it;

	for (it = global_scope->begin(); it != global_scope->end(); it++) {
		Type	*t;

		t = dynamic_cast<Type*>(*it);
		if (t == NULL)
			continue;

		/* XXX condition bitmap should not be zero, should
		 * the condition bitmap be exposed at this level? */
		types[t->getName()] = new PhysTypeUser(t, 0);
	}
}
#endif

static void dump_resolved(const Type* t)
{
	PhysicalType	*pt;
	Expr		*bytes;
	Expr		*bits;

	pt = t->resolve(types);

	cout << "Type \"" << t->getName() << "\". Bytes = " << endl;
}

int main(int argc, char *argv[])
{
	GlobalBlock::iterator	it;

	yyparse();

	load_primitive_types();

	for (it = global_scope->begin(); it != global_scope->end(); it++) {
		GlobalStmt	*gs = *it;
		Type		*t;

		cout << *gs << endl << endl;

		t = dynamic_cast<Type*>(gs);
		if (t == NULL) continue;

		t->resolve(types);
	}

	return 0;
}
