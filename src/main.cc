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

static void load_user_types_thunk(const GlobalBlock* gb);
static void load_primitive_types(void);

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

static void load_user_types_thunk(GlobalBlock* gb)
{
	GlobalBlock::iterator	it;

	for (it = gb->begin(); it != gb->end(); it++) {
		Type	*t;

		t = dynamic_cast<Type*>(*it);
		if (t == NULL)
			continue;

		/* XXX condition bitmap should not be zero, should
		 * the condition bitmap be exposed at this level? */
		types[string("thunk_") + t->getName()] = new PhysTypeThunk(t);
	}
}

static void load_user_types_resolved(GlobalBlock* gb)
{
	GlobalBlock::iterator	it;

	for (it = gb->begin(); it != gb->end(); it++) {
		Type	*t;

		t = dynamic_cast<Type*>(*it);
		if (t == NULL)
			continue;

		if (types.count(t->getName()) != 0) {
			cerr << t->getName() << " already declared!" << endl;
		}

		cout << "resolving " << t->getName() << endl;

		types[t->getName()] = new PhysTypeUser(t, t->resolve(types));
	}


	/* now, do it again to get the missing thunks */
	for (it = gb->begin(); it != gb->end(); it++) {
		Type	*t;

		t = dynamic_cast<Type*>(*it);
		if (t == NULL)
			continue;

		delete types[t->getName()];

		cout << "resolving " << t->getName() << endl;

		types[t->getName()] = new PhysTypeUser(t, t->resolve(types));
	}
}

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
	load_user_types_thunk(global_scope);
	load_user_types_resolved(global_scope);

	for (it = global_scope->begin(); it != global_scope->end(); it++) {
		GlobalStmt	*gs = *it;
		Type		*t;
		PhysicalType	*pt;
		Expr		*e;

		cout << *gs << endl << endl;

		t = dynamic_cast<Type*>(gs);
		if (t == NULL) continue;

		pt = types[t->getName()];
		assert (pt != NULL);

		e = pt->getBytes();
		assert (e != NULL);

		e->print(cout);
		cout << endl;
		delete e;
	}

	return 0;
}
