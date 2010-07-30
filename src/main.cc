#include <iostream>
#include <string>
#include <map>
#include <typeinfo>
#include "AST.h"
#include "type.h"
#include "func.h"
#include "phys_type.h"
#include "symtab.h"
#include "eval.h"
#include "code_builder.h"

#include <stdint.h>
#include <fstream>

extern int yyparse();

extern GlobalBlock* global_scope;
extern void gen_rt_tables(void);

using namespace std;

static func_map		funcs_map;
static func_list	funcs_list;
ptype_map		ptypes_map;	/* XXX add to evalctx? */
const Func		*gen_func;
const FuncBlock		*gen_func_block;
ctype_map		ctypes_map;
type_map		types_map;
type_list		types_list;
const_map		constants;
symtab_map		symtabs;
llvm_var_map		thunk_var_map;

CodeBuilder		*code_builder;


static void	load_user_types_list(const GlobalBlock* gb);
static bool	load_user_ptypes_thunk(void);
static void	load_primitive_ptypes(void);
static void	build_inline_symtabs(void);
static bool	apply_consts_to_consts(void);
static void	simplify_constants(void);
static void	build_thunk_symtabs(void);
static void	create_global(const char* str, uint64_t v);

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
		assert (ptypes_map.count(pt[i]->getName()) == 0);
		ptypes_map[pt[i]->getName()] = pt[i];
	}

	ptypes_map["bool"] = new U1(); 
	ptypes_map["int"] = new I64();
	ptypes_map["uint"] = new U64();

	for (	ptype_map::const_iterator it = ptypes_map.begin();
		it != ptypes_map.end();
		it++)
	{
		PhysicalType	*pt;
		Expr		*bits, *bits2;
		Number		*n;

		pt = (*it).second;

		bits = pt->getBits();
		bits2 = bits->simplify();
		delete bits;
		bits = bits2;

		n = dynamic_cast<Number*>(bits);
		assert (n != NULL);

		ctypes_map[(*it).first] = n->getValue();

		delete bits;
	}
}

static void load_user_types_list(const GlobalBlock* gb)
{
	GlobalBlock::const_iterator	it;
	int				type_num;

	type_num = 0;
	for (it = gb->begin(); it != gb->end(); it++) {
		Type	*t;

		t = dynamic_cast<Type*>(*it);
		if (t == NULL) continue;

		types_list.push_back(t);
		types_map[t->getName()] = t;

		t->setTypeNum(type_num);
		type_num++;
	}

	code_builder->createGlobalConst("fsl_num_types", type_num);
	/* remember to set these in the run-time */
	code_builder->createGlobalMutable("__FROM_OS_BDEV_BYTES", 0);
	code_builder->createGlobalMutable("__FROM_OS_BDEV_BLOCK_BYTES", 0);
}

static void load_user_funcs(const GlobalBlock* gb)
{
	GlobalBlock::const_iterator	it;

	for (it = gb->begin(); it != gb->end(); it++) {
		Func		*f;

		f = dynamic_cast<Func*>(*it);
		if (f == NULL) continue;

		/* add to mappings.. */
		funcs_list.push_back(f);
		funcs_map[f->getName()] = f;

		cout << "LOADING FUNC: " << f->getName() << endl;
		gen_func_code(f);
	}
}

void dump_ptypes(void)
{
	for (	ptype_map::const_iterator p_it = ptypes_map.begin();
		p_it != ptypes_map.end();
		p_it++)
	{
		cerr << (*p_it).first << endl;
	}

}

/**
 * build up symbol tables, disambiguate if possible
 */
static void build_symtabs(void)
{
	type_list::iterator	it;

	for (it = types_list.begin(); it != types_list.end(); it++) {
		Type		*t;
		SymbolTable	*syms;

		t = *it;
		assert (t != NULL);

		cout << "Building symtab for " << t->getName() << endl;

		t->buildSyms();
		syms = t->getSyms();
		symtabs[t->getName()] = syms;
	}
}

static void load_constants(const GlobalBlock* gb)
{
	GlobalBlock::const_iterator	it;
	
	constants["true"] = new Number(1);
	constants["false"] = new Number(0);

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
		Expr			*new_expr, *old_expr;
		
		old_expr = (p.second)->copy();
		new_expr = expr_resolve_consts(constants, p.second);

		if (*old_expr != new_expr) {
			constants[p.first] = new_expr;
			updated = true;
		}

		delete old_expr;
	}

	return updated;
}

#define NUM_RUNTIME_FUNCS	8
/* TODO: __max should be a proper vararg function */
const char*	f_names[] = {	
	"__getLocal", "__getLocalArray", "__getDyn", "fsl_fail", 
	"__max2", "__max3", "__max4", "__max5"};
int	f_arg_c[] = {
	2,4,1,0, 
	2,3,4,5};

/**
 * insert run-time functions into the llvm module so that they resolve
 * when called..
 */
static void load_runtime_funcs(void)
{
	for (int i = 0; i < NUM_RUNTIME_FUNCS; i++) {
		vector<const llvm::Type*>	args;
		llvm::FunctionType		*ft;
		llvm::Function			*f;

		args = vector<const llvm::Type*>(
			f_arg_c[i], 
			llvm::Type::getInt64Ty(llvm::getGlobalContext()));

		ft = llvm::FunctionType::get(
			llvm::Type::getInt64Ty(llvm::getGlobalContext()),
			args,
			false);
	
		f = llvm::Function::Create(
			ft,
			llvm::Function::ExternalLinkage, 
			f_names[i],
			code_builder->getModule());
	}
}

/**
 * go through type dumping all usertype fields
 */
static void dump_usertype_fields(const Type* t)
{
	SymbolTable	*st;

	cout << "Getting Syms By User Type.." << endl;

	st = t->getSymsByUserType();
	assert (st != NULL);

	cout << "DUMPING: " << t->getName() << endl;
	for (	sym_map::const_iterator it = st->begin();
		it != st->end();
		it++)
	{
		string			name = (*it).first;
		const SymbolTableEnt*	ent = (*it).second;

		/* only dump strong fields */
		if (ent->isWeak() == false) 
			cout << "FIELD: " << name << endl;
	}
	cout << "------------------------" << endl;

	delete st;
}

/**
 * go through all types, dumping user types
 */
static void dump_usertypes(void)
{
	for (	type_list::const_iterator it = types_list.begin();
		it != types_list.end();
		it++)
	{
		const Type	*t = *it;
		dump_usertype_fields(t);
	}
}

static void gen_thunk_code(void)
{
	for (	symtab_map::const_iterator it = symtabs.begin();
		it != symtabs.end();
		it++)
	{
		const SymbolTable	*st;
		const ThunkType		*thunk_type;

		st = (*it).second;

		thunk_type = st->getThunkType();
		thunk_type->genCode();
	}
}

static void gen_thunk_proto(void)
{
	for (	symtab_map::const_iterator it = symtabs.begin();
		it != symtabs.end();
		it++)
	{
		const SymbolTable	*st;
		const ThunkType		*thunk_type;

		st = (*it).second;
		thunk_type = st->getThunkType();
		thunk_type->genProtos();
	}
}

int main(int argc, char *argv[])
{
	llvm::LLVMContext&	ctx = llvm::getGlobalContext();
	ofstream		os("fsl.types.ll");

	code_builder = new CodeBuilder("fsl.types.mod");

	yyparse();

	/* create prototypes for functions provided by the run-time */
	load_runtime_funcs();

	/* load ptypes in */
	cout << "Loading ptypes" << endl;
	load_primitive_ptypes();

	/* make consts resolve to numbers */
	cout << "Setting up constants" << endl;
	load_constants(global_scope);
	load_enums(global_scope);
	cout << "Simplifying constants" << endl;
	simplify_constants();

	cout << "Loading user types list" << endl;
	load_user_types_list(global_scope);

	/* next, build up symbol tables on types.. this is our type checking */
	cout << "Building symbol tables" << endl;
	build_symtabs();


	cout << "Generating thunk prototypes" << endl;
	gen_thunk_proto();

	/* generate function prototyes so that we can resolve thunks within
	 * functions */
	cout << "Loading user functions" << endl;
	load_user_funcs(global_scope);

	cout << "Loading thunks" << endl;
	gen_thunk_code();

	code_builder->write(os);

	cout << "OK. Dumping user types: " << endl;
	dump_usertypes();

	cout << "Generating fsl.table.c" << endl;
	gen_rt_tables();

	return 0;
}
