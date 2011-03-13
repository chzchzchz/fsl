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

Value* Id::codeGen(void) const
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
	if (ai == NULL) ai = code_builder->getTmpAllocaInst(getName());

	if (ai == NULL) {
		cerr << "Could not alloc tmp inst" << endl;
		code_builder->printTmpVars();
		return ErrorV(
			string("Could not find variable ") +
			getName());
	}

	return get_builder()->CreateLoad(ai, getName());
}

Value* IdStruct::codeGen(void) const
{
	print(cerr);
	cerr << endl;
	return ErrorV("XXX: STUB: IdStruct");
}

Value* IdArray::codeGen(void) const { return ErrorV("IdArray"); }
Value* ExprParens::codeGen(void) const { return expr->codeGen(); }

#define AOP_CODEGEN(x,y) 						\
Value* x::codeGen(void) const { 					\
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

Value* CondVal::codeGen(void) const
{
	llvm::BasicBlock	*bb_then, *bb_else, *bb_merge, *bb_origin;
	llvm::PHINode		*pn;
	llvm::Function		*f;
	llvm::IRBuilder<>	*builder;
	llvm::LLVMContext	&gctx(llvm::getGlobalContext());
	llvm::Value		*c_v, *e_t_v, *e_f_v;

	builder = code_builder->getBuilder();
	bb_origin = builder->GetInsertBlock();
	f = bb_origin->getParent();

	bb_then = llvm::BasicBlock::Create(gctx, "cv_then", f);
	bb_else = llvm::BasicBlock::Create(gctx, "cv_else", f);
	bb_merge = llvm::BasicBlock::Create(gctx, "cv_merge", f);

	builder->SetInsertPoint(bb_origin);

	c_v = ce->codeGen();

	builder->CreateCondBr(c_v, bb_then, bb_else);
	builder->SetInsertPoint(bb_then);
	e_t_v = e_t->codeGen();
	builder->CreateBr(bb_merge);

	builder->SetInsertPoint(bb_else);
	e_f_v = e_f->codeGen();
	builder->CreateBr(bb_merge);

	builder->SetInsertPoint(bb_merge);
	pn = builder->CreatePHI(llvm::Type::getInt64Ty(gctx), "cv_iftmp");
	pn->addIncoming(e_t_v, bb_then);
	pn->addIncoming(e_f_v, bb_else);

	return pn;
}

CondVal::~CondVal(void)
{
	delete ce;
	delete e_t;
	delete e_f;
}

CondVal* CondVal::copy(void) const
{
	return new CondVal(ce->copy(), e_t->copy(), e_f->copy());
}
