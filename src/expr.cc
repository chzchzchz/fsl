#include <stdint.h>
#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/IRBuilder.h>
#include <iostream>

#include "expr.h"

using namespace std;
using namespace llvm;

extern IRBuilder<>	*builder;
extern Module		*mod;

Value* Expr::ErrorV(const char* s) const
{
	cerr << "expr error: " << s << " in ";
	print(cerr);
	cerr << endl;
	return NULL;
}

static Value* createCond(CmpInst::Predicate cmp, const FCall* fc)
{
	const ExprList			*exprs;
	ExprList::const_iterator	it;
	Expr				*lhs, *rhs;
	Expr				*is_true, *is_false;
	Value				*condV;
	Value				*is_true_v, *is_false_v;
	BasicBlock			*bb_if, *bb_else, *bb_merge;
	Function			*llvm_f;
	PHINode				*phi;

	exprs = fc->getExprs();
	it = exprs->begin();

	lhs = *(it++);
	rhs = *(it++);
	is_true = *(it++);
	is_false = *(it++);

	/* UGH. */

	condV = builder->CreateICmp(
		cmp,
		lhs->codeGen(),
		rhs->codeGen());

	llvm_f = builder->GetInsertBlock()->getParent();
	
	bb_if = BasicBlock::Create(getGlobalContext(), "then");
	bb_else = BasicBlock::Create(getGlobalContext(), "else");
	bb_merge = BasicBlock::Create(getGlobalContext(), "ifcont");

	builder->CreateCondBr(condV, bb_if, bb_else);
	
	builder->SetInsertPoint(bb_if);
	is_true_v = is_true->codeGen();
	builder->CreateBr(bb_merge);
	bb_if = builder->GetInsertBlock();

	llvm_f->getBasicBlockList().push_back(bb_else);
	builder->SetInsertPoint(bb_else);
	is_false_v = is_false->codeGen();

	builder->CreateBr(bb_merge);
	bb_else = builder->GetInsertBlock();

	llvm_f->getBasicBlockList().push_back(bb_merge);
	builder->SetInsertPoint(bb_merge);
	phi = builder->CreatePHI(Type::getInt64Ty(getGlobalContext()), "iftmp");

	phi->addIncoming(is_true_v, bb_if);
	phi->addIncoming(is_false_v, bb_else);

	return phi;
}

/** TODO: This should be able to catch user-defined functions! */
Value* FCall::codeGen() const
{
	Function			*callee;
	ExprList::const_iterator	it;
	vector<Value*>			args;
	string				call_name(id->getName());

	/* TODO: make this cleaner */
	if (call_name == "cond_eq") {
		return createCond(CmpInst::ICMP_EQ, this);
	} else if (call_name == "cond_ne") {
		return createCond(CmpInst::ICMP_NE, this);
	}

	callee = mod->getFunction(call_name);
	if (callee == NULL) {
		return ErrorV(
			(string("could not find function ") + 
			id->getName()).c_str());
	}

	if (callee->arg_size() != exprs->size()) {
		return ErrorV(
			(string("wrong number of function arguments ") + 
			id->getName()).c_str());		
	}

	for (it = exprs->begin(); it != exprs->end(); it++) {
		Expr	*cur_expr;
		cur_expr = *it;
		args.push_back(cur_expr->codeGen());
	}

	return builder->CreateCall(callee, args.begin(), args.end(), "calltmp");
}

Value* Id::codeGen() const 
{
	return ErrorV("XXX: STUB: Id");
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
	return builder->CreateOr(e_lhs->codeGen(), e_rhs->codeGen());
}

Value* AOPAnd::codeGen() const
{
	return builder->CreateAnd(e_lhs->codeGen(), e_rhs->codeGen());
}

Value* AOPAdd::codeGen() const
{
	return builder->CreateAdd(e_lhs->codeGen(), e_rhs->codeGen());
}

Value* AOPSub::codeGen() const
{
	return builder->CreateSub(e_lhs->codeGen(), e_rhs->codeGen());
}

Value* AOPDiv::codeGen() const
{
	return builder->CreateUDiv(e_lhs->codeGen(), e_rhs->codeGen());
}

Value* AOPMul::codeGen() const
{
	return builder->CreateMul(e_lhs->codeGen(), e_rhs->codeGen());
}

Value* AOPLShift::codeGen() const
{
	return builder->CreateShl(e_lhs->codeGen(), e_rhs->codeGen());
}

Value* AOPRShift::codeGen() const
{
	return builder->CreateLShr(e_lhs->codeGen(), e_rhs->codeGen());
}

Value* AOPMod::codeGen() const
{
	return builder->CreateURem(e_lhs->codeGen(), e_rhs->codeGen());
}
