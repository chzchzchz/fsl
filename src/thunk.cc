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

#include  "thunk.h"

#include <stdint.h>
#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/IRBuilder.h>

using namespace std;

llvm_var_map			thunk_var_map;

extern type_list		types_list;
extern ptype_map		ptypes_map;	/* XXX add to evalctx? */
extern const_map		constants;
extern symtab_map		symtabs_inlined;

extern llvm::Module		*mod;
extern llvm::IRBuilder<> 	*builder;


static void gen_thunk_args(const Type* t, llvm::Function* f);


static void gen_thunk_code_from_type(
	const Type* t,
	llvm::Function* &thunk_bytes, llvm::Function* &thunk_bits);
static void gen_thunk_proto_from_type(Type* t);
static void gen_thunkfield_proto_from_type(const Type* t);

static void gen_thunkfield_code_from_type(const Type* t);
static void gen_thunkfield_code_from_typefield(
	const Type* t, 
	const string& var_name,
	const SymbolTable* local_syms,
	llvm::Function* &thunk_bits);


static void gen_proto_by_name(const Type* t, string fcall_name);
static void gen_header_args(const Type* t, llvm::Function* f);

void gen_thunk_proto(void)
{
	type_list::const_iterator	it;

	for (type_list::const_iterator it = types_list.begin(); 
	     it != types_list.end(); 
	     it++) {
		Type	*t;

		t = *it;
		cout << "Generating thunk proto: " << t->getName() << endl;
		gen_thunk_proto_from_type(t);
	}
}

void gen_thunkfield_proto(void)
{
	for (	type_list::iterator it = types_list.begin(); 
		it != types_list.end();
		it++)
	{
		cout << "GEN THUNKOFF PROTO " << (*it)->getName() << endl;
		gen_thunkfield_proto_from_type(*it);
	}
}



void gen_thunk_code(void)
{
	for (	type_list::const_iterator it = types_list.begin();
		it != types_list.end();
		it++) 
	{
		Type		*t;
		llvm::Function	*type_thunk_bits_f, *type_thunk_bytes_f;

		t = *it;
		cerr << "Generating thunk: " << t->getName() << endl;

		gen_thunk_code_from_type(
			t, type_thunk_bits_f, type_thunk_bytes_f);
		type_thunk_bytes_f->dump();
	}
}

void gen_thunkfield_code(void)
{
	for (	type_list::iterator it = types_list.begin(); 
		it != types_list.end();
		it++)
	{
		cout << "GEN THUNKOFF PROTO " << (*it)->getName() << endl;
		gen_thunkfield_code_from_type(*it);
	}
}


/**
 * create function for a type
 */
static void gen_thunk_code_from_type(
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
	local_syms->copyInto(*(symtabs_inlined[t->getName()]), off_name);
	delete off_name;

	expr_eval_bits = eval(
		EvalCtx(*local_syms, symtabs_inlined, constants),
		expr_bits);

	expr_eval_bytes = eval(
		EvalCtx(*local_syms, symtabs_inlined, constants),
		expr_bytes);



	bb_bytes = llvm::BasicBlock::Create(
		llvm::getGlobalContext(), "entry", f_bytes);

	bb_bits = llvm::BasicBlock::Create(
		llvm::getGlobalContext(), "entry", f_bits);

	builder->SetInsertPoint(bb_bytes);
	gen_header_args(t, f_bytes);
	builder->CreateRet(expr_eval_bytes->codeGen());

	builder->SetInsertPoint(bb_bits);
	gen_header_args(t, f_bits);
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



static void gen_proto_by_name(const Type* t, string fcall_name)
{
	llvm::Function		*f;
	llvm::FunctionType	*ft;
	PhysicalType		*pt;
	vector<const llvm::Type*>	args(
		t->getNumArgs() + 1	/* extra arg for offset */, 
		llvm::Type::getInt64Ty(llvm::getGlobalContext()));

	ft = llvm::FunctionType::get(
		llvm::Type::getInt64Ty(llvm::getGlobalContext()),
		args,
		false);

	f = llvm::Function::Create(
		ft,
		llvm::Function::ExternalLinkage, 
		fcall_name,
		mod);

	/* should not be redefinitions.. */
	assert (f->getName() == fcall_name);
}

class UnionThunkProtoGen :  protected TypeVisitAll
{
public:
	UnionThunkProtoGen() : cur_type (NULL) {}

	virtual void genUnionProtos(const Type* t) 
	{
		assert (cur_type == NULL);
		cout << "WOOOOOOOOOO" << endl;
		cur_type = t;
		apply(t);
		cur_type = NULL;
	}

	virtual void visit(const TypeUnion* tu)
	{
		cerr << "GENERATING: " << 
			(string("__thunk___union_")
				+ cur_type->getName()
				+ "_" + tu->getName()
				+ "_bits") << endl;
		gen_proto_by_name(
			cur_type, 
			string("__thunk___union_")
				+ cur_type->getName()
				+ "_" + tu->getName()
				+ "_bits");
		gen_proto_by_name(
			cur_type, 
			string("__thunk___union_")
				+ cur_type->getName()
				+ "_" + tu->getName()
				+ "_bytes");

		TypeVisitAll::visit(tu);
	}

	virtual ~UnionThunkProtoGen() { assert (cur_type == NULL); }
protected:
private:
	const Type* cur_type;
};

static void gen_thunk_proto_from_type(Type* t)
{
	UnionThunkProtoGen	*union_thunk_gen;

	gen_proto_by_name(
		t, string("__thunk_") + t->getName() + "_bits");
	gen_proto_by_name(
		t, string("__thunk_") + t->getName() + "_bytes");

	union_thunk_gen = new UnionThunkProtoGen();
	union_thunk_gen->genUnionProtos(t);
	delete union_thunk_gen;
}


static void gen_thunkfield_code_from_typefield(
	const Type* t, 
	const string& var_name,
	const SymbolTable* local_syms,
	llvm::Function* &thunk_bits)
{
	llvm::Function		*f_bits;
	llvm::BasicBlock	*bb_bits;
	string			fcall_bits;
	Expr			*expr_bits;
	Expr			*expr_eval_bits;
	PhysicalType		*pt;
	sym_binding		sb;

	
	fcall_bits = typeOffThunkName(t, var_name);
	f_bits = mod->getFunction(fcall_bits);
	assert (f_bits != NULL);

	if ((symtabs_inlined[t->getName()])->lookup(var_name, sb) == false) {
		/* should always exist */
		assert (0 == 1);
	}

	expr_bits = symbind_off(sb);

	expr_eval_bits = eval(
		EvalCtx(*local_syms, symtabs_inlined, constants),
		expr_bits);

	cerr << "GENERATED OFFTHUNK FOR " << var_name << endl;
	expr_bits->print(cerr);
	cerr << endl << endl;

	bb_bits = llvm::BasicBlock::Create(
		llvm::getGlobalContext(), "entry", f_bits);


	builder->SetInsertPoint(bb_bits);
	gen_header_args(t, f_bits);
	builder->CreateRet(expr_eval_bits->codeGen());

	delete expr_bits;
	delete expr_eval_bits;

	thunk_bits = f_bits;

	thunk_var_map.clear();
}

/**
 * generate *code* for all offsets in type
 */
static void gen_thunkfield_code_from_type(const Type* t)
{
	SymbolTable		*st;
	SymbolTable		*local_syms;
	string			type_name(t->getName());
	sym_map::const_iterator	it;

	st = symtabs_inlined[t->getName()];
	assert (st != NULL);

	/* protos should have already been generated. */

	local_syms = new SymbolTable(NULL, NULL);
	Id	*off_name = new Id("__thunk_off_arg");
	local_syms->copyInto(*st, off_name);
	delete off_name;

	/* generate code for all offsets */
	for (it = st->begin(); it != st->end(); it++) {
		string		field_name((*it).first);
		llvm::Function	*f_bits;

		cerr << "GENERATING THUNK OFF CODE FOR " << field_name << endl;
		gen_thunkfield_code_from_typefield(
			t, field_name, local_syms, f_bits);
	}

	delete local_syms;
}

static void gen_thunkfield_proto_from_type(const Type* t)
{
	SymbolTable		*st;
	string			type_name(t->getName());
	sym_map::const_iterator	it;

	st = symtabs_inlined[t->getName()];
	assert (st != NULL);

	for (it = st->begin(); it != st->end(); it++) {
		string			name((*it).first);
		sym_binding		sb((*it).second);
		const PhysicalType	*pta;

		gen_proto_by_name(
			t, 
			typeOffThunkName(t, name));

		pta = dynamic_cast<const PhysTypeArray*>(symbind_phys(sb));
		if (pta != NULL) {
			/* generate thunkfield length */

		}
	}
}

static void gen_header_args(const Type* t, llvm::Function* f)
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


