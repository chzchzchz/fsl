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
ptype_map		ptypes_map;	/* XXX add to evalctx? */
llvm_var_map		thunk_var_map;
const Func		*gen_func;
const FuncBlock		*gen_func_block;
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

static llvm::Function* gen_func_code_proto(const Func* f);
static bool gen_func_code_args(
	const Func* f, vector<const llvm::Type*>& llvm_args);


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

static bool gen_func_code_args(
	const Func* f, vector<const llvm::Type*>& llvm_args)
{
	const ArgsList	*args;

	args = f->getArgs();
	assert (args != NULL);

	llvm_args.clear();

	for (unsigned int i = 0; i < args->size(); i++) {
		string		cur_type(((args->get(i)).first)->getName());
		PhysicalType		*pt;
		const llvm::Type	*t;

		pt = ptypes_map[cur_type];
		if (pt == NULL) {
			cerr << f->getName() << 
				": Could not resolve argument type for \"" <<
				cur_type << '"' << endl;
			return false;
		}

		t = pt->getLLVMType();
		if (t == NULL) {
			cerr << f->getName() << ": bad func arg type \"" << 
			cur_type << "\"." << endl;
			return false;
		}

		llvm_args.push_back(t);
	}

	return true;
}

static llvm::Function* gen_func_code_proto(const Func* f)
{
	vector<const llvm::Type*>	f_args;
	llvm::Type			*ret_type;
	llvm::Function			*llvm_f;
	llvm::FunctionType		*llvm_ft;
	PhysicalType			*pt_ret;
	const llvm::Type		*t_ret;

	pt_ret = ptypes_map[f->getRet()];
	if (pt_ret == NULL) {
		cerr	<< "Bad return type (" << f->getRet() << ") for "
			<< f->getName() << endl;
		return NULL;
	}
	t_ret = pt_ret->getLLVMType();

	if (gen_func_code_args(f, f_args) == false) {
		cerr << "Bailing on generating " << f->getName() << endl;
		return NULL;
	}

	/* llvm stuff..*/
	llvm_ft = llvm::FunctionType::get(t_ret, f_args, false);
	llvm_f = llvm::Function::Create(
		llvm_ft, llvm::Function::ExternalLinkage, f->getName(), mod);
	if (llvm_f->getName() != f->getName()) {
		cerr << f->getName() << " already declared!" << endl;
		return NULL;
	}

	return llvm_f;
}

static void gen_func_code(Func* f)
{
	EvalCtx*			ectx;
	SymbolTable*			cur_scope;
	llvm::Function			*llvm_f;
	llvm::BasicBlock		*f_bb;

	llvm_f = gen_func_code_proto(f);
	if (llvm_f == NULL)
		return;

	cur_scope = new SymbolTable(NULL, NULL);
	cur_scope->loadArgs(ptypes_map, f->getArgs());

	/* scope takes args */
	ectx = new EvalCtx(*cur_scope, symtabs, constants);
	
	f_bb = llvm::BasicBlock::Create(
		llvm::getGlobalContext(), "entry", llvm_f);

	builder->SetInsertPoint(f_bb);

	/* set gen_func so that expr id resolution will get the right
	 * alloca variable */
	gen_func = f;
	f->codeGen(ectx);
	gen_func = NULL;
	gen_func_block = NULL;

	delete ectx;
	delete cur_scope;

	llvm_f->dump();
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
			new PhysTypeThunk(t);
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
		if (ptypes_map.count(t->getName()) != 0) {
			cerr << t->getName() << " already declared!" << endl;
			success = false;
			continue;
		}


		ptypes_map[t->getName()] = new PhysTypeUser(
			t, t->resolve(base, ptypes_map));
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

		cout << "Building symtab for " << t->getName() << endl;

		syms = t->getSyms(ptypes_map);
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

extern bool xxx_debug_eval;

static void gen_thunk_args(const Type* t, llvm::Function* f)
{
	unsigned int			arg_c;
	const ArgsList			*args;
	const llvm::Type		*l_t;
	llvm::AllocaInst		*allocai;
	llvm::Function::arg_iterator	ai;
	llvm::IRBuilder<>		tmpB(
		&f->getEntryBlock(), f->getEntryBlock().begin());

	l_t = llvm::Type::getInt64Ty(llvm::getGlobalContext());
	ai = f->arg_begin();
	args = t->getArgs();
	arg_c = (args != NULL) ? args->size() : 0;

	thunk_var_map.clear();

	/* create the hidden argument __thunk_off_arg */
	allocai = tmpB.CreateAlloca(l_t, 0, "__thunk_off_arg");
	thunk_var_map["__thunk_off_arg"] = allocai;
	thunk_var_map[t->getName()] = allocai;	/* alias for typename */
	builder->CreateStore(ai, allocai);

	/* create the rest of the arguments */
	ai++;
	for (unsigned int i = 0; i < arg_c; i++, ai++) {
		string			arg_name;
		arg_name = (args->get(i).second)->getName();
		allocai = tmpB.CreateAlloca(l_t, 0, arg_name);
		builder->CreateStore(ai, allocai);
		thunk_var_map[arg_name] = allocai;
	}

}

/**
 * create function for a type
 */
static void thunk_from_type(
	const Type* t,
	llvm::Function* &thunk_bytes, llvm::Function* &thunk_bits)
{
	llvm::Function		*f_bytes, *f_bits;
	llvm::BasicBlock	*bb_bits, *bb_bytes;
	string			fcall_bits, fcall_bytes;
	Expr			*expr_bits, *expr_bytes;
	Expr			*expr_eval_bits, *expr_eval_bytes;
	PhysicalType		*pt;
	SymbolTable		*local_syms;

	
	fcall_bits = (string("__thunk_") + t->getName()) + "_bits";
	fcall_bytes = (string("__thunk_") + t->getName()) + "_bytes";

	f_bytes = mod->getFunction(fcall_bytes);
	f_bits = mod->getFunction(fcall_bits);

	pt = ptypes_map[t->getName()];
	assert (pt != NULL);

	expr_bits = pt->getBits();
	expr_bytes = pt->getBytes();

	local_syms = new SymbolTable(NULL, NULL);
	Id	*off_name = new Id("__thunk_off_arg");
	local_syms->copyInto(*(symtabs[t->getName()]), off_name);
	delete off_name;

	expr_eval_bits = eval(
		EvalCtx(*local_syms, symtabs, constants),
		expr_bits);

	expr_eval_bytes = eval(
		EvalCtx(*local_syms, symtabs, constants),
		expr_bytes);



	bb_bytes = llvm::BasicBlock::Create(
		llvm::getGlobalContext(), "entry", f_bytes);

	bb_bits = llvm::BasicBlock::Create(
		llvm::getGlobalContext(), "entry", f_bits);

	builder->SetInsertPoint(bb_bytes);
	gen_thunk_args(t, f_bytes);
	builder->CreateRet(expr_eval_bytes->codeGen());

	builder->SetInsertPoint(bb_bits);
	gen_thunk_args(t, f_bits);
	builder->CreateRet(expr_eval_bits->codeGen());

	delete local_syms;
	delete expr_bits;
	delete expr_bytes;
	delete expr_eval_bits;
	delete expr_eval_bytes;

	thunk_bytes = f_bytes;
	thunk_bits = f_bits;

	thunk_var_map.clear();
}

static void gen_thunk_proto(Type* t)
{
	llvm::Function		*f_bytes, *f_bits;
	llvm::FunctionType	*ft;
	string			fcall_bits, fcall_bytes;
	PhysicalType		*pt;
	vector<const llvm::Type*>	args(
		t->getNumArgs() + 1	/* extra arg for offset */, 
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
}

static void gen_thunks(void)
{
	type_list::const_iterator it;

	for (it = types_list.begin(); it != types_list.end(); it++) {
		Type	*t;

		t = *it;
		cout << "Generating thunk proto: " << t->getName() << endl;
		gen_thunk_proto(t);
	}

	for (it = types_list.begin(); it != types_list.end(); it++) {
		Type		*t;
		llvm::Function	*type_thunk_bits_f, *type_thunk_bytes_f;

		t = *it;
		cerr << "Generating thunk: " << t->getName() << endl;

		thunk_from_type(t, type_thunk_bits_f, type_thunk_bytes_f);
		type_thunk_bytes_f->dump();
	}

}

#define NUM_RUNTIME_FUNCS	6
const char*	f_names[] = {	
	"__getLocal", "__getLocalArray", "__arg", "__getDyn", "fsl_fail_bits",
	"fsl_fail_bytes"};
int	f_arg_c[] = {2, 4, 1, 1, 0, 0};

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

	builder = new llvm::IRBuilder<>(ctx);
	mod = new llvm::Module("mod", ctx);

	yyparse();

	load_runtime_funcs();

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

	cout << "Loading user functions" << endl;
	load_user_funcs(global_scope);

	gen_thunks();

	return 0;
}
