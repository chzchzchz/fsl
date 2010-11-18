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

#define get_builder()	code_builder->getBuilder()

Value* Expr::ErrorV(const string& s) const
{
	cerr << endl;
	cerr << "expr error: " << s << " in the expression \"";
	print(cerr);
	cerr << "\"" << endl;

	cerr << "LLVM DUMP:" << endl;
	code_builder->getBuilder()->GetInsertBlock()->getParent()->dump();

	assert (0 == 1 && "GENERAL ERROR");
	return NULL;
}


Value* Id::codeGen() const 
{
	AllocaInst		*ai = NULL;
	GlobalVariable		*gv;

	if (getName() == "__NULLPTR")
		return code_builder->getNullPtrI64();

	/* load.. */
	if (gen_func_block == NULL && code_builder->getTmpVarCount() == 0) {
		return ErrorV(
			string("Loading ") + getName() + 
			string(" with no gen_func set"));
	}

	if (gen_func_block != NULL)
		ai = gen_func_block->getVar(getName());
	else if ((gv = code_builder->getGlobalVar(getName())) != NULL)
		return get_builder()->CreateLoad(gv, getName());

	/* create variable, if one does not already exist */
	if (ai == NULL)
		ai = code_builder->getTmpAllocaInst(getName());

	if (ai == NULL) {
		cerr << "OH NO: EXPR ERROR" << endl;
		code_builder->printTmpVars();
		return ErrorV(
			string("Could not find variable ") + 
			getName());
	}

	return get_builder()->CreateLoad(ai, getName());
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
	return get_builder()->CreateOr(e_lhs->codeGen(), e_rhs->codeGen());
}

Value* AOPAnd::codeGen() const
{
	return get_builder()->CreateAnd(e_lhs->codeGen(), e_rhs->codeGen());
}

Value* AOPAdd::codeGen() const
{
	return get_builder()->CreateAdd(e_lhs->codeGen(), e_rhs->codeGen());
}

Value* AOPSub::codeGen() const
{
	return get_builder()->CreateSub(e_lhs->codeGen(), e_rhs->codeGen());
}

Value* AOPDiv::codeGen() const
{
	return get_builder()->CreateUDiv(e_lhs->codeGen(), e_rhs->codeGen());
}

Value* AOPMul::codeGen() const
{
	return get_builder()->CreateMul(e_lhs->codeGen(), e_rhs->codeGen());
}

Value* AOPLShift::codeGen() const
{
	return get_builder()->CreateShl(
		e_lhs->codeGen(), e_rhs->codeGen());
}

Value* AOPRShift::codeGen() const
{
	return get_builder()->CreateLShr(e_lhs->codeGen(), e_rhs->codeGen());
}

Value* AOPMod::codeGen() const
{
	return get_builder()->CreateURem(e_lhs->codeGen(), e_rhs->codeGen());
}

Value* Boolean::codeGen() const
{
	return ConstantInt::get(getGlobalContext(), APInt(1, b, false));
}
