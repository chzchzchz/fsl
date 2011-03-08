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
extern const VarScope	*gen_vscope;

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

	if (getName() == "__NULLPTR") return code_builder->getNullPtrI64();
	if (getName() == "__NULLPTR8") return code_builder->getNullPtrI8();

	if (gen_vscope != NULL) {
		if ((ai = gen_vscope->getVar(getName())) != NULL)
			return get_builder()->CreateLoad(ai, getName());
	}

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

Value* IdArray::codeGen() const { return ErrorV("XXX: STUB: IdArray"); }
Value* ExprParens::codeGen() const { return expr->codeGen(); }

#define AOP_CODEGEN(x,y) 						\
Value* x::codeGen() const { 						\
	return get_builder()->y(e_lhs->codeGen(), e_rhs->codeGen());	\
}

AOP_CODEGEN(AOPOr, CreateOr);
AOP_CODEGEN(AOPAnd, CreateAnd);
AOP_CODEGEN(AOPAdd, CreateAdd);
AOP_CODEGEN(AOPSub, CreateSub);
AOP_CODEGEN(AOPDiv, CreateUDiv);
AOP_CODEGEN(AOPMul, CreateMul);
AOP_CODEGEN(AOPLShift, CreateShl);
AOP_CODEGEN(AOPRShift, CreateLShr);
AOP_CODEGEN(AOPMod, CreateURem);

Value* Number::codeGen() const
{
	return ConstantInt::get(getGlobalContext(), APInt(64, n, false));
}

Value* Boolean::codeGen() const
{
	return ConstantInt::get(getGlobalContext(), APInt(1, b, false));
}
