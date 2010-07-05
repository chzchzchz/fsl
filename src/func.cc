#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/IRBuilder.h>

#include "cond.h"
#include "func.h"
#include "eval.h"

using namespace std;
using namespace llvm;

extern IRBuilder<>	*builder;
extern Module		*mod;
extern ptype_map	ptypes_map;
extern const FuncBlock	*gen_func_block;

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

	e_v = evalAndGen(*ectx, expr);

	if (e_v == NULL) {
		cerr << getLineNo() << ": Could not eval ";
		expr->print(cerr);
		cerr << endl;
		return NULL;

	}

	/* XXX no support for arrays just yet */
	assert (array == NULL);

	builder->CreateStore(e_v, getOwner()->getVar(scalar->getName()));

	return e_v;
}

Value* FuncRet::codeGen(const EvalCtx* ectx) const
{
	Value	*e_v;	

	e_v = evalAndGen(*ectx, expr);
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
