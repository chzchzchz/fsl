#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/IRBuilder.h>

#include "cond.h"
#include "func.h"
#include "eval.h"
#include "funcevalctx.h"

using namespace std;
using namespace llvm;

extern IRBuilder<>	*builder;
extern Module		*mod;
extern ptype_map	ptypes_map;
extern const FuncBlock	*gen_func_block;
extern const Func	*gen_func;
extern const_map	constants;
extern symtab_map	symtabs;

static bool gen_func_code_args(
		const Func* f,
		vector<const llvm::Type*>& llvm_args);
static Function* gen_func_code_proto(const Func* f);


Func* FuncStmt::getFunc(void) const
{
	if (f_owner == NULL)
		return owner->getFunc();

	return f_owner;
}

Value* Func::codeGen(const EvalCtx* ectx) const
{
	Function			*f;
	Function::arg_iterator		ai;
	unsigned int			arg_c;

	assert (ectx != NULL);

	/* create arguments */
	f = getFunction();
	ai = f->arg_begin();
	arg_c = args->size();
	IRBuilder<>			tmpB(
		&f->getEntryBlock(), f->getEntryBlock().begin());

	for (unsigned int i = 0; i < arg_c; i++, ai++) {
		AllocaInst		*allocai;
		PhysicalType		*pt;
		const llvm::Type	*t;
		string			arg_name;

		arg_name = (args->get(i).second)->getName();
		pt = ptypes_map[(args->get(i).first)->getName()];
		t = pt->getLLVMType();
		allocai = tmpB.CreateAlloca(t, 0, arg_name);
		block->addVar(arg_name, allocai);
		builder->CreateStore(ai, allocai);
	}

	/* gen rest of code */
	return block->codeGen(ectx);
}

Value* FuncDecl::codeGen(const EvalCtx* ectx) const
{
	FuncBlock		*scope;
	Function		*f = getFunc()->getFunction();
	PhysicalType		*pt;
	const llvm::Type	*t;
	AllocaInst		*ai;
	IRBuilder<>	tmpB(&f->getEntryBlock(), f->getEntryBlock().begin());

	/* XXX no support for arrays yet */
	assert (array == NULL);

	pt = ptypes_map[type->getName()];
	if (pt == NULL || (t = pt->getLLVMType()) == NULL) {
		cerr << getLineNo() << ": could not resolve type for "
			<< type->getName();
		return NULL;
	}

	ai = tmpB.CreateAlloca(t, 0, scalar->getName());

	scope = getOwner();
	if (scope->addVar(scalar->getName(), ai) == false) {
		cerr << "Already added variable " << scalar->getName() << endl;
		return NULL;
	}

	return NULL;
}

Value* FuncAssign::codeGen(const EvalCtx* ectx) const
{
	Value	*e_v;

	cerr << "ASSIGN!: ";
	expr->print(cerr);
	cerr << endl;
	e_v = evalAndGen(*ectx, expr);
	cerr << "ASSIGN SUCCESSSSSSSSSS!" << endl;

	if (e_v == NULL) {
		cerr << getLineNo() << ": Could not eval ";
		expr->print(cerr);
		cerr << endl;
		return NULL;

	}

	/* XXX no support for arrays just yet */
	assert (array == NULL);

	cerr << "Creating the store.." << endl;
	cerr << "Assigning to: " << scalar->getName() << endl;

	builder->CreateStore(e_v, getOwner()->getVar(scalar->getName()));


	return e_v;
}

Value* FuncRet::codeGen(const EvalCtx* ectx) const
{
	Value	*e_v;	

	cerr << "RET!: ";
	expr->print(cerr);
	cerr << endl;

	e_v = evalAndGen(*ectx, expr);
	cerr << "SUCCESSFUL GENERATION! YES!" << endl;
	if (e_v == NULL) {
		cerr << getLineNo() << ": Could not eval ";
		expr->print(cerr);
		cerr << endl;
	}

	builder->CreateRet(e_v);

	return e_v;
}

Value* FuncCondStmt::codeGen(const EvalCtx* ectx) const
{
	Function	*f;
	BasicBlock	*bb_then, *bb_else, *bb_merge;
	Value		*cond_v;

	f = getFunction();
	bb_then = BasicBlock::Create(getGlobalContext(), "then", f);
	bb_else = BasicBlock::Create(getGlobalContext(), "else");
	bb_merge = BasicBlock::Create(getGlobalContext(), "ifcont");
	cond_v = cond_codeGen(ectx, cond);
	if (cond_v == NULL) {
		cerr << getLineNo() << ": could not gen condition" << endl;
		return NULL;
	}

	/*
	 * if (cond_v) {
	 * 	(bb_then)
	 * 	jmp merge
	 * } else {
	 * 	(bb_else)
	 * 	jmp merge
	 * }
	 * merge:
	 */
	builder->CreateCondBr(cond_v, bb_then, bb_else);
	builder->SetInsertPoint(bb_then);
	is_true->codeGen(ectx);
	builder->CreateBr(bb_merge);

	builder->SetInsertPoint(bb_else);
	if (is_false != NULL)
		is_false->codeGen(ectx);
	builder->CreateBr(bb_merge);

	builder->SetInsertPoint(bb_merge);

//	f->getBasicBlockList().push_back(bb_then);

	f->getBasicBlockList().push_back(bb_else);
	f->getBasicBlockList().push_back(bb_merge);

	return cond_v;
}

Value* FuncBlock::codeGen(const EvalCtx* ectx) const
{
	for (const_iterator it = begin(); it != end(); it++) {
		FuncStmt	*fstmt = *it;
		gen_func_block = this;
		fstmt->codeGen(ectx);
	}

	return NULL;
}


AllocaInst* FuncBlock::getVar(const std::string& s) const
{
	map<string, AllocaInst*>::const_iterator	it;

	it = vars.find(s);
	if (it == vars.end()) {
		if (getOwner() == NULL)
			return NULL;

		return getOwner()->getVar(s);
	}

	return (*it).second;
}

bool FuncBlock::addVar(const std::string& name, llvm::AllocaInst* ai)
{
	if (vars.count(name) != 0)
		return false;

	vars[name] = ai;
	return true;
}

Function* Func::getFunction(void) const
{
	return mod->getFunction(getName());
}

Function* FuncStmt::getFunction() const
{
	return getFunc()->getFunction();
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

void gen_func_code(Func* f)
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
	ectx = new FuncEvalCtx(*cur_scope, symtabs, constants);
	
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


