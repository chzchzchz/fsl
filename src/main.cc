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

#include <stdint.h>
#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/IRBuilder.h>

extern int yyparse();

extern GlobalBlock* global_scope;

using namespace std;

static func_map		funcs_map;
static func_list	funcs_list;
static ptype_map	ptypes;
static type_list	types_list;
static type_map		types_map;
static const_map	constants;
static symtab_map	symtabs;

llvm::Module	*mod;
llvm::IRBuilder<> *builder;


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

		types_list.push_back(t);
		types_map[t->getName()] = t;

		t->setTypeNum(type_num);
		type_num++;
	}	
}

static void load_user_funcs(const GlobalBlock* gb)
{
	GlobalBlock::const_iterator	it;

	for (it = gb->begin(); it != gb->end(); it++) {
		Func*	f;

		f = dynamic_cast<Func*>(*it);
		if (f == NULL) continue;

		funcs_list.push_back(f);
		funcs_map[f->getName()] = f;
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
	Expr			*base = new Number(0);

	success = true;
	for (it = types_list.begin(); it != types_list.end(); it++) {
		Type	*t;

		t = *it;
		if (ptypes.count(t->getName()) != 0) {
			cerr << t->getName() << " already declared!" << endl;
			success = false;
			continue;
		}


		ptypes[t->getName()] = new PhysTypeUser(
			t, t->resolve(base, ptypes));
	}


	/* now, do it again to get the non-parameterized thunks */
	for (it = types_list.begin(); it != types_list.end(); it++) {
		Type	*t;

		t = *it;

		delete ptypes[t->getName()];

		cout << "resolving " << t->getName() << endl;

		ptypes[t->getName()] = new PhysTypeUser(
			t, t->resolve(base, ptypes));

	}

	delete base;

	return success;
}

/**
 * build up symbol tables, disambiguate if possible
 */
static void build_sym_tables(void)
{
	type_list::iterator	it;

	for (it = types_list.begin(); it != types_list.end(); it++) {
		Type		*t;
		SymbolTable	*syms;

		t = *it;
		assert (t != NULL);

		cout << "Building " << t->getName() << endl;

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
//			cerr << "DONE WITH SIMPLIFY" << endl;
//			dump_constants();
//			cerr << "_-----------------------__" << endl;
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
			if (*old_expr != cur_expr)
				updated = true;
		} else {
			delete cur_expr;
			constants[p.first] = new_expr;
			updated = true;
		}

		delete old_expr;
	}

	return updated;
}

/**
 * create function for a type
 */
static void thunk_from_type(
	const Type* t,
	llvm::Function* &thunk_bytes, llvm::Function* &thunk_bits)
{
	llvm::Function		*f_bytes, *f_bits;
	llvm::FunctionType	*ft;
	llvm::BasicBlock	*bb_bits, *bb_bytes;
	string			fcall_bits, fcall_bytes;
	Expr			*expr_bits, *expr_bytes;
	Expr			*expr_eval_bits, *expr_eval_bytes;
	PhysicalType		*pt;
	vector<const llvm::Type*>	args(
		t->getNumArgs(), 
		llvm::Type::getInt64Ty(llvm::getGlobalContext()));

	ft = llvm::FunctionType::get(
		llvm::Type::getInt64Ty(llvm::getGlobalContext()),
		args,
		false);
	
	fcall_bits = (string("__thunk_") + t->getName()) + "_bits";
	fcall_bytes = (string("__thunk_") + t->getName()) + "_bytes";

	f_bytes = llvm::Function::Create(
		ft,
		llvm::Function::ExternalLinkage, 
		fcall_bytes,
		mod);

	f_bits = llvm::Function::Create(
		ft,
		llvm::Function::ExternalLinkage, 
		fcall_bits,
		mod);

	/* should not be redefinitions.. */
	assert (f_bytes->getName() == fcall_bytes);
	assert (f_bits->getName() == fcall_bits);

	pt = ptypes[t->getName()];
	assert (pt != NULL);

	expr_bits = pt->getBits();
	expr_bytes = pt->getBytes();

	expr_eval_bits = eval(
		EvalCtx(*(symtabs[t->getName()]),
			symtabs,
			constants), expr_bits);
				
	expr_eval_bytes = eval(
		EvalCtx(*(symtabs[t->getName()]),
			symtabs,
			constants), expr_bytes);



	bb_bytes = llvm::BasicBlock::Create(
		llvm::getGlobalContext(), "entry", f_bytes);
	bb_bits = llvm::BasicBlock::Create(
		llvm::getGlobalContext(), "entry", f_bits);

	builder->SetInsertPoint(bb_bytes);
	builder->CreateRet(expr_eval_bytes->codeGen());

	builder->SetInsertPoint(bb_bits);
	builder->CreateRet(expr_eval_bits->codeGen());

	delete expr_bits;
	delete expr_bytes;
	delete expr_eval_bits;
	delete expr_eval_bytes;

	thunk_bytes = f_bytes;
	thunk_bits = f_bits;
}

static void gen_thunks(void)
{
	/* XXX -- Generate code for all type thunks! */
	for (	type_list::const_iterator it = types_list.begin();
		it != types_list.end();
		it++)
	{
		Type		*t;
		llvm::Function	*type_thunk_bits_f, *type_thunk_bytes_f;

		t = *it;
		cout << "Generating thunk: " << t->getName() << endl;

		thunk_from_type(t, type_thunk_bits_f, type_thunk_bytes_f);

		type_thunk_bytes_f->dump();
	}
}

#define NUM_RUNTIME_FUNCS	4
const char*	f_names[] = {	
	"__getLocal", "__getLocalArray", "__arg", "__getDyn"};
int	f_arg_c[] = {2, 4, 1, 3};

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

	yyparse();

	/* load ptypes in */
	cout << "Loading ptypes" << endl;
	load_primitive_ptypes();
	cout << "Loading user types" << endl;
	load_user_types(global_scope);
	cout << "Loading thunks" << endl;
	load_user_ptypes_thunk();
	cout << "Resolving user types" << endl;
	load_user_ptypes_resolved();

	/* next, build up symbol tables on types.. this is our type checking */
	cout << "Building symbol tables" << endl;
	build_sym_tables();

	/* make consts resolve to numbers */
	cout << "Setting up constants" << endl;
	load_constants(global_scope);
	load_enums(global_scope);
	cout << "Simplifying constants" << endl;
	simplify_constants();

	builder = new llvm::IRBuilder<>(ctx);
	mod = new llvm::Module("mod", ctx);

	load_runtime_funcs();

	gen_thunks();

	return 0;
}
