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
#include "thunk.h"

#include <stdint.h>
#include <fstream>

#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Support/raw_os_ostream.h>

extern int yyparse();

extern GlobalBlock* global_scope;

using namespace std;

static func_map		funcs_map;
static func_list	funcs_list;
ptype_map		ptypes_map;	/* XXX add to evalctx? */
const Func		*gen_func;
const FuncBlock		*gen_func_block;
static type_map		types_map;
type_list		types_list;
const_map		constants;
symtab_map		symtabs_inlined;
symtab_map		symtabs_thunked;

llvm::Module		*mod;
llvm::IRBuilder<> 	*builder;


static void	load_user_types_list(const GlobalBlock* gb);
static bool	load_user_ptypes_thunk(void);
static bool	load_user_ptypes_resolved(void);
static void	load_primitive_ptypes(void);
static void	build_inline_symtabs(void);
static bool	apply_consts_to_consts(void);
static void	simplify_constants(void);
static void	build_thunk_symtabs(void);
static void	create_global(const char* str, uint64_t v);


#define create_global_const(x, y) create_global(x, y, true)
#define create_global_mutable(x, y) create_global(x, y, false)

static void create_global(const char* str, uint64_t v, bool is_const)
{
	llvm::GlobalVariable	*gv;

	gv = new llvm::GlobalVariable(
		*mod,
		llvm::Type::getInt64Ty(llvm::getGlobalContext()),
		is_const,
		llvm::GlobalVariable::ExternalLinkage,
		llvm::ConstantInt::get(
			llvm::getGlobalContext(), 
			llvm::APInt(64, v, false)),
		str);
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
		assert (ptypes_map.count(pt[i]->getName()) == 0);
		ptypes_map[pt[i]->getName()] = pt[i];
	}

	ptypes_map["bool"] = new U64(); 
	ptypes_map["int"] = new I64();
	ptypes_map["uint"] = new U64();
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

	create_global_const("fsl_num_types", type_num);

	/* remember to set these in the run-time */
	create_global_mutable("__FROM_OS_BDEV_BYTES", 0);
	create_global_mutable("__FROM_OS_BDEV_BLOCK_BYTES", 0);

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

		cout << "generating function " << f->getName() << endl;
		gen_func_code(f);
	}
}

static bool load_user_ptypes_thunk(void)
{
	type_list::iterator	it;
	bool			success;

	success = true;
	for (it = types_list.begin(); it != types_list.end(); it++) {
		Type		*t;
		string		thunk_str;

		t = *it;
		thunk_str = string("thunk_") + t->getName();
		if (ptypes_map.count(thunk_str) != 0) {
			cerr << "Type \"" << t->getName() <<
				"\" already declared!" << endl;
			success = false;
			continue;
		}

		ptypes_map[string("thunk_") + t->getName()] = 
			new PhysTypeThunk(t, new ExprList(new Id("PT_THUNK_ARG")));
	}

	return success;
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

static bool load_user_ptypes_resolved(void)
{
	type_list::iterator	it;
	bool			success;
	Expr			*base = new Number(0);

	success = true;
	for (it = types_list.begin(); it != types_list.end(); it++) {
		Type	*t;

		t = *it;
		cout << "adding.. " << t->getName() << endl;
		if (ptypes_map.count(t->getName()) != 0) {
			cerr << t->getName() << " already declared!" << endl;
			success = false;
			continue;
		}

		ptypes_map[t->getName()] = new PhysTypeUser(
			t, t->resolve(ptypes_map));
	}


	/* now, do it again to get the non-parameterized thunks */
	/* XXX we don't want to inline parameterized thunks because otherwise
	 * we run into some horrific scoping issues. 
	 * On that note, we still run into issues even *without* non-param
	 * thunks because if we have align() then that will cause conflicts
	 * on the offset... */
#if 0
	for (it = types_list.begin(); it != types_list.end(); it++) {
		Type	*t;

		t = *it;

		delete ptypes_map[t->getName()];

		cout << "resolving " << t->getName() << endl;

		ptypes_map[t->getName()] = new PhysTypeUser(
			t, t->resolve(base, ptypes_map));

	}
#endif
	delete base;

	return success;
}


static void build_thunk_symtabs(void)
{
	type_list::iterator it;
	for (it = types_list.begin(); it != types_list.end(); it++) {
		Type		*t;
		t = *it;
		cout << "Building thunk symtab for " << t->getName() << endl;
		t->buildSymsThunked(ptypes_map);
		symtabs_thunked[t->getName()] = t->getSymsThunked(ptypes_map);
	}
}


/**
 * build up symbol tables, disambiguate if possible
 */
static void build_inline_symtabs(void)
{
	type_list::iterator	it;

	for (it = types_list.begin(); it != types_list.end(); it++) {
		Type		*t;
		SymbolTable	*syms;

		t = *it;
		assert (t != NULL);

		cout << "Building symtab for " << t->getName() << endl;

		t->buildSyms(ptypes_map);
		syms = t->getSyms(ptypes_map);
		symtabs_inlined[t->getName()] = syms;
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

#define NUM_RUNTIME_FUNCS	9
/* TODO: __max should be a proper vararg function */
const char*	f_names[] = {	
	"__getLocal", "__getLocalArray", "__getDyn", "fsl_fail_bits", 
	"__max2", "__max3", "__max4", "__max5", "fsl_fail_bytes"};
int	f_arg_c[] = {
	2,4,1,0, 
	2,3,4,5,0 };

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
			mod);
	}
}

int main(int argc, char *argv[])
{
	llvm::LLVMContext&	ctx = llvm::getGlobalContext();
	ofstream		os("fsl.types.out");
	llvm::raw_os_ostream	llvm_out(os);

	builder = new llvm::IRBuilder<>(ctx);
	mod = new llvm::Module("mod", ctx);

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

	cout << "Creating user type sym thunks" << endl;
	load_user_ptypes_thunk();

	cout << "Building thunk symbol tables" << endl;
	build_thunk_symtabs();
	
	cout << "Resolving user types into expressions" << endl;
	load_user_ptypes_resolved();

	/* next, build up symbol tables on types.. this is our type checking */
	cout << "Building inlined symbol tables" << endl;
	build_inline_symtabs();


	cout << "Generating thunk prototypes" << endl;
	gen_thunk_proto();
	gen_thunkfield_proto();

	/* generate function prototyes so that we can resolve thunks within
	 * functions */
	cout << "Loading user functions" << endl;
	load_user_funcs(global_scope);

	cout << "Loading thunks" << endl;
	gen_thunk_code();
	gen_thunkfield_code();

	mod->print(llvm_out, NULL);

	return 0;
}
