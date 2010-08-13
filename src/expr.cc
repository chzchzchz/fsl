#include <stdint.h>
#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/IRBuilder.h>
#include <iostream>

#include "code_builder.h"
#include "func.h"
#include "expr.h"
#include "util.h"

using namespace std;
using namespace llvm;

extern CodeBuilder	*code_builder;
extern const Func	*gen_func;
extern const FuncBlock	*gen_func_block;

Value* Expr::ErrorV(const string& s) const
{
	cerr << endl;
	cerr << "expr error: " << s << " in the expression \"";
	print(cerr);
	cerr << "\"" << endl;
	assert (0 == 1);
	return NULL;
}

/** TODO: This should be able to catch user-defined functions! */
Value* FCall::codeGen() const
{
	Function			*callee;
	ExprList::const_iterator	it;
	vector<Value*>			args;
	string				call_name(id->getName());

	callee = code_builder->getModule()->getFunction(call_name);
	if (callee == NULL) {
		return ErrorV(
			(string("could not find function ") + 
			id->getName()).c_str());
	}

	if (callee->arg_size() != exprs->size()) {
		return ErrorV(
			(string("wrong number of function arguments in ") + 
			id->getName() + 
			string(". Expected ") + 
			int_to_string(callee->size()) + 
			" got " + int_to_string(exprs->size())).c_str());
	}

	for (it = exprs->begin(); it != exprs->end(); it++) {
		Expr	*cur_expr;
		cur_expr = *it;
		args.push_back(cur_expr->codeGen());
	}

	return code_builder->getBuilder()->CreateCall(
		callee, args.begin(), args.end(), "calltmp");
}

Value* Id::codeGen() const 
{
	AllocaInst		*ai;
	GlobalVariable		*gv;

	/* load.. */
	if (gen_func_block == NULL && code_builder->getThunkVarCount() == 0) {
		return ErrorV(
			string("Loading ") + getName() + 
			string(" with no gen_func set"));
	}

	if (gen_func_block != NULL)
		ai = gen_func_block->getVar(getName());
	else if ((gv = code_builder->getGlobalVar(getName())) != NULL) {
		return code_builder->getBuilder()->CreateLoad(gv, getName());
	} else
		ai = code_builder->getThunkAllocInst(getName());

	if (ai == NULL) {
		return ErrorV(
			string("Could not find variable ") + 
			getName());
	}

	return code_builder->getBuilder()->CreateLoad(ai, getName());
}

Value* IdStruct::codeGen() const 
{
	print(cerr);
	cerr << endl;
	return ErrorV("XXX: STUB: IdStruct");
}

Value* IdArray::codeGen() const 
{
	return ErrorV("XXX: STUB: IdArray");
}

Value* ExprParens::codeGen() const 
{
	return expr->codeGen();
}

Value* Number::codeGen() const 
{
	return ConstantInt::get(getGlobalContext(), APInt(64, n, false));
}

Value* AOPOr::codeGen() const
{
	return code_builder->getBuilder()->CreateOr(
		e_lhs->codeGen(), e_rhs->codeGen());
}

Value* AOPAnd::codeGen() const
{
	return code_builder->getBuilder()->CreateAnd(
		e_lhs->codeGen(), e_rhs->codeGen());
}

Value* AOPAdd::codeGen() const
{
	return code_builder->getBuilder()->CreateAdd(
		e_lhs->codeGen(), e_rhs->codeGen());
}

Value* AOPSub::codeGen() const
{
	return code_builder->getBuilder()->CreateSub(
		e_lhs->codeGen(), e_rhs->codeGen());
}

Value* AOPDiv::codeGen() const
{
	return code_builder->getBuilder()->CreateUDiv(
		e_lhs->codeGen(), e_rhs->codeGen());
}

Value* AOPMul::codeGen() const
{
	return code_builder->getBuilder()->CreateMul(
		e_lhs->codeGen(), e_rhs->codeGen());
}

Value* AOPLShift::codeGen() const
{
	return code_builder->getBuilder()->CreateShl(
		e_lhs->codeGen(), e_rhs->codeGen());
}

Value* AOPRShift::codeGen() const
{
	return code_builder->getBuilder()->CreateLShr(
		e_lhs->codeGen(), e_rhs->codeGen());
}

Value* AOPMod::codeGen() const
{
	return code_builder->getBuilder()->CreateURem(
		e_lhs->codeGen(), e_rhs->codeGen());
}
