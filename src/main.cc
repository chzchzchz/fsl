#include <iostream>
#include <string>
#include <map>
#include <typeinfo>
#include "AST.h"
#include "type.h"
#include "phys_type.h"
#include "symtab.h"

extern int yyparse();

extern GlobalBlock* global_scope;

using namespace std;

ptype_map			ptypes;
type_list			types;
const_map			constants;
symtab_map			symtabs;

static void	load_user_types(const GlobalBlock* gb);
static bool	load_user_ptypes_thunk(void);
static bool	load_user_ptypes_resolved(void);
static void	load_primitive_ptypes(void);
static void	build_sym_tables(void);
static bool	apply_consts_to_consts(void);
static void	simplify_constants(void);

extern void eval(
	const Expr* expr,
	const const_map& consts,
	const symtab_map& symtabs
	);

extern Expr* expr_resolve_consts(const const_map& consts, Expr* cur_expr);



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
	int				type_num;

	type_num = 0;
	for (it = gb->begin(); it != gb->end(); it++) {
		Type	*t;

		t = dynamic_cast<Type*>(*it);
		if (t == NULL) continue;

		types.push_back(t);
		t->setTypeNum(type_num);
		type_num++;
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
		assert (t != NULL);

		syms = t->getSyms(ptypes);
		symtabs[t->getName()] = syms;
	}
}

static void load_constants(const GlobalBlock* gb)
{
	GlobalBlock::const_iterator	it;

	for (it = gb->begin(); it != gb->end(); it++) {
		ConstVar	*c;

		c = dynamic_cast<ConstVar*>(*it);
		if (c == NULL)
			continue;

		constants[c->getName()] = (c->getExpr())->copy();
	}
}

static void load_enums(const GlobalBlock* gb)
{
	GlobalBlock::const_iterator	it;

	for (it = gb->begin(); it != gb->end(); it++) {
		Enum::iterator	eit;
		Enum		*e;
		unsigned long	n;

		e = dynamic_cast<Enum*>(*it);
		if (e == NULL)
			continue;

		n = 0;
		for (eit = e->begin(); eit != e->end(); eit++) {
			const EnumEnt		*ent;
			const Expr		*ent_num;
			Expr			*num;

			ent = *eit;
			ent_num = ent->getNumber();
			if (ent_num == NULL) {
				num = new Number(n);
			} else {
				num = ent_num->copy();
			}

			constants[ent->getName()] = num;
			n++;
		}
	}
}

#define MAX_PASSES	100

static void dump_constants(void)
{
	for (	const_map::iterator it = constants.begin(); 
		it!=constants.end(); 
		it++) 
	{
		pair<string, Expr*>	p(*it);

		cerr << p.first << ": ";
		p.second->print(cerr);
		cerr << endl;
	}
}

static void simplify_constants(void)
{
	unsigned int pass;

	pass = 0;
	while (pass < MAX_PASSES) {
		if (!apply_consts_to_consts()) {
			cerr << "DONE WITH SIMPLIFY" << endl;
			dump_constants();
			cerr << "_-----------------------__" << endl;
			return;
		}
		pass++;
	}

	cerr << "Could not resolve constants after " << pass << 
		"passes -- circular?" << endl;
	cerr << "Dumping: " << endl;
	dump_constants();

	assert (0 == 1);
}

static bool apply_consts_to_consts(void)
{

	const_map::iterator	it;
	bool			updated;

	updated = false;
	for (it = constants.begin(); it != constants.end(); it++){
		pair<string, Expr*>	p(*it);
		Expr			*new_expr, *old_expr, *cur_expr;
		
		cur_expr = p.second;
		old_expr = cur_expr->copy();
		new_expr = expr_resolve_consts(constants, p.second);

		if (new_expr == NULL) {
			if (*old_expr != cur_expr) {
				cout << "HELO" << endl;
				old_expr->print(cout);
				cout << " vs ";
				cur_expr->print(cout);
				cout << endl;
				updated = true;
			}
		} else {
			delete cur_expr;
			constants[p.first] = new_expr;
			updated = true;
		}

		delete old_expr;
	}

	return updated;
}

static void dump_ptypes(GlobalBlock* gb)
{
	GlobalBlock::iterator	it;
	/* dump ptypes.. */
	for (it = gb->begin(); it != gb->end(); it++) {
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
}

static void gen_thunks(void)
{
	/* XXX -- Generate code for all type thunks! */
	assert (0 == 1);
}


int main(int argc, char *argv[])
{
	yyparse();

	/* load ptypes in */
	load_primitive_ptypes();
	load_user_types(global_scope);
	load_user_ptypes_thunk();
	load_user_ptypes_resolved();

	/* next, build up symbol tables on types.. this is our type checking */
	build_sym_tables();

	/* make consts resolve to numbers */
	load_constants(global_scope);
	load_enums(global_scope);
	simplify_constants();

//	eval();

	dump_ptypes(global_scope);

	return 0;
}
