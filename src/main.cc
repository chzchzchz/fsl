#include <iostream>
#include <string>
#include <map>
#include "AST.h"
#include "type.h"
#include "phys_type.h"
#include "symtab.h"

extern int yyparse();

extern GlobalBlock* global_scope;

using namespace std;

typedef list<Type*>	type_list;

map<string, SymbolTable*>	symtabs;
ptype_map			ptypes;
type_list			types;

static void load_user_types(const GlobalBlock* gb);
static bool load_user_ptypes_thunk(void);
static bool load_user_ptypes_resolved(void);
static void load_primitive_ptypes(void);
static void build_sym_tables(void);

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

static void load_primitive_ptypes(void)
{
	PhysicalType	*pt[] = {
		new U1(),
		new U8(),
		new U16(),
		new U32(),
		new U64(),
		new I16(),
		new I32(),
		new I64(),
		new U12()};
	
	for (unsigned int i = 0; i < (sizeof(pt)/sizeof(PhysicalType*)); i++) {
		assert (ptypes.count(pt[i]->getName()) == 0);
		ptypes[pt[i]->getName()] = pt[i];
	}
}

static void load_user_types(const GlobalBlock* gb)
{
	GlobalBlock::const_iterator	it;

	for (it = gb->begin(); it != gb->end(); it++) {
		Type	*t;

		t = dynamic_cast<Type*>(*it);
		if (t == NULL) continue;

		types.push_back(t);
	}	
}

static bool load_user_ptypes_thunk(void)
{
	type_list::iterator	it;
	bool			success;

	success = true;
	for (it = types.begin(); it != types.end(); it++) {
		Type		*t;
		string		thunk_str;

		t = *it;
		thunk_str = string("thunk_") + t->getName();
		if (ptypes.count(thunk_str) != 0) {
			cerr << "Type \"" << t->getName() <<
				"\" already declared!" << endl;
			success = false;
			continue;
		}

		ptypes[string("thunk_") + t->getName()] = new PhysTypeThunk(t);
	}

	return success;
}

static bool load_user_ptypes_resolved(void)
{
	type_list::iterator	it;
	bool			success;

	success = true;
	for (it = types.begin(); it != types.end(); it++) {
		Type	*t;

		t = *it;
		if (ptypes.count(t->getName()) != 0) {
			cerr << t->getName() << " already declared!" << endl;
			success = false;
			continue;
		}


		cout << "resolving " << t->getName() << endl;

		ptypes[t->getName()] = new PhysTypeUser(t, t->resolve(ptypes));
	}


	/* now, do it again to get the non-parameterized thunks */
	for (it = types.begin(); it != types.end(); it++) {
		Type	*t;

		t = *it;

		delete ptypes[t->getName()];

		cout << "resolving " << t->getName() << endl;

		ptypes[t->getName()] = new PhysTypeUser(t, t->resolve(ptypes));
	}

	return success;
}

static void dump_resolved(const Type* t)
{
	PhysicalType	*pt;
	Expr		*bytes;
	Expr		*bits;

	pt = t->resolve(ptypes);

	cout << "Type \"" << t->getName() << "\". Bytes = " << endl;
}

/**
 * build up symbol tables, disambiguate if possible
 */
static void build_sym_tables(void)
{
	type_list::iterator	it;

	for (it = types.begin(); it != types.end(); it++) {
		Type		*t;
		SymbolTable	*syms;

		t = *it;

		syms = t->getSyms(ptypes);
		symtabs[t->getName()] = syms;
		syms->print(cout);
	}
}

int main(int argc, char *argv[])
{
	GlobalBlock::iterator	it;

	yyparse();

	/* load ptypes in */
	load_primitive_ptypes();
	load_user_types(global_scope);
	load_user_ptypes_thunk();
	load_user_ptypes_resolved();

	/* next, build up symbol tables on types.. this is our type checking */
	build_sym_tables();

	/* also, verify that ptypes are correct for arguments */
	

	/* dump ptypes.. */
	for (it = global_scope->begin(); it != global_scope->end(); it++) {
		GlobalStmt	*gs = *it;
		Type		*t;
		PhysicalType	*pt;
		Expr		*e;

		cout << *gs << endl << endl;

		t = dynamic_cast<Type*>(gs);
		if (t == NULL) continue;

		pt = ptypes[t->getName()];
		assert (pt != NULL);

		e = pt->getBytes();
		assert (e != NULL);

		e->print(cout);
		cout << endl;
		delete e;
	}

	return 0;
}
