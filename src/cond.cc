#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/IRBuilder.h>
#include <assert.h>

#include "phys_type.h"
#include "expr.h"
#include "cond.h"

extern llvm::IRBuilder<>	*builder;


static PhysicalType* cond_cmpop_resolve(
	const CmpOp*		cmpop,
	const ptype_map&	tm,
	PhysicalType		*t,
	PhysicalType		*f);

static PhysicalType* cond_bop_resolve(
	const BinBoolOp*	bop,
	const ptype_map& 	tm,
	PhysicalType*		t,
	PhysicalType*		f);

static PhysicalType* cond_cmpop_resolve(
	const CmpOp*		cmpop,
	const ptype_map&	tm,
	PhysicalType		*t,
	PhysicalType		*f)
{
	Expr		*lhs, *rhs;

	assert (cmpop != NULL);

	lhs = (cmpop->getLHS())->simplify();
	rhs = (cmpop->getRHS())->simplify();

	switch (cmpop->getOp()) {
	case CmpOp::EQ: return new PhysTypeCondEQ(lhs, rhs, t, f);
	case CmpOp::NE: return new PhysTypeCondNE(lhs, rhs, t, f);
	case CmpOp::LE: return new PhysTypeCondLE(lhs, rhs, t, f);
	case CmpOp::LT: return new PhysTypeCondLT(lhs, rhs, t, f);
	case CmpOp::GT: return new PhysTypeCondGT(lhs, rhs, t, f);
	case CmpOp::GE: return new PhysTypeCondGE(lhs, rhs, t, f);
	default:
		assert (0 == 1);
	}

	return NULL;
}

static PhysicalType* cond_bop_resolve(
	const BinBoolOp*	bop,
	const ptype_map& 	tm,
	PhysicalType*		t,
	PhysicalType*		f)
{
	const CondExpr*	cond_lhs;
	const CondExpr*	cond_rhs;

	cond_lhs = bop->getLHS();
	cond_rhs = bop->getRHS();

	assert (bop != NULL);

	if ((dynamic_cast<const BOPAnd*>(bop)) != NULL) {
		/* if LHS evaluates to true, do RHS */

		return cond_resolve(
			cond_lhs,
			tm,
			cond_resolve(cond_rhs, tm, t, f), 
			f);
	} else {
		/* must be an OR */
		assert (dynamic_cast<const BOPOr*>(bop) != NULL);

		return cond_resolve(
			cond_lhs,
			tm,
			t,
			cond_resolve(cond_rhs, tm, t, f));
	}

	/* should not happen */
	assert (0 == 1);
	return NULL;
}

/**
 * convert a condition into a physical type.
 */
PhysicalType* cond_resolve(
	const CondExpr* cond, 
	const ptype_map& tm,
	PhysicalType* t, PhysicalType* f)
{
	const CmpOp	*cmpop;
	const BinBoolOp	*bop;
	const FuncCond	*fcond;

	cmpop = dynamic_cast<const CmpOp*>(cond);
	if (cmpop != NULL) {
		return cond_cmpop_resolve(cmpop, tm, t, f);
	}

	bop = dynamic_cast<const BinBoolOp*>(cond);
	if (bop != NULL) 
		return cond_bop_resolve(bop, tm, t, f);

	fcond = dynamic_cast<const FuncCond*>(cond);
	assert (fcond != NULL);

	return new PhysTypeCondEQ(
		(fcond->getFC())->simplify(),
		new Number(1),
		t,
		f);
}

using namespace std;

llvm::Value* cond_cmpop_codeGen(const EvalCtx* ctx, const CmpOp* cmpop)
{
	llvm::Value			*lhs, *rhs;
	llvm::CmpInst::Predicate	pred;


	assert (cmpop != NULL);

	lhs = evalAndGen(*ctx, cmpop->getLHS());
	rhs = evalAndGen(*ctx, cmpop->getRHS());

	switch (cmpop->getOp()) {
	case CmpOp::EQ: pred = llvm::CmpInst::ICMP_EQ; break;
	case CmpOp::NE: pred = llvm::CmpInst::ICMP_NE; break;
	case CmpOp::LE: pred = llvm::CmpInst::ICMP_ULE; break;
	case CmpOp::LT: pred = llvm::CmpInst::ICMP_ULT; break;
	case CmpOp::GT: pred = llvm::CmpInst::ICMP_UGT; break;
	case CmpOp::GE: pred = llvm::CmpInst::ICMP_UGE; break;
	default:
		assert (0 == 1);
	}

	return builder->CreateICmp(pred, lhs, rhs);
}

llvm::Value* cond_bop_and_codeGen(const EvalCtx* ctx, const BOPAnd* bop_and)
{
	const CondExpr		*lhs, *rhs;
	llvm::Value		*lhs_v;
	llvm::BasicBlock	*bb_then, *bb_else, *bb_merge;
	llvm::PHINode		*pn;
	llvm::Function		*f;

	f = builder->GetInsertBlock()->getParent();

	lhs = bop_and->getLHS();
	rhs = bop_and->getRHS();
	
	bb_then = llvm::BasicBlock::Create(
		llvm::getGlobalContext(), "then", f);
	bb_else = llvm::BasicBlock::Create(
		llvm::getGlobalContext(), "else");
	bb_merge = llvm::BasicBlock::Create(
		llvm::getGlobalContext(), "merge");

	lhs_v = cond_codeGen(ctx, lhs);
	/*
	 * if (lhs_v) {
	 * 	(bb_then)
	 * 	jmp merge
	 * } else {
	 * 	(bb_else)
	 * 	jmp merge
	 * }
	 * merge:
	 */
	builder->CreateCondBr(lhs_v, bb_then, bb_else);
	builder->SetInsertPoint(bb_then);
	builder->CreateBr(bb_merge);
	builder->SetInsertPoint(bb_else);
	builder->CreateBr(bb_merge);
	builder->SetInsertPoint(bb_merge);

	f->getBasicBlockList().push_back(bb_then);
	f->getBasicBlockList().push_back(bb_else);
	f->getBasicBlockList().push_back(bb_merge);

	pn->addIncoming(cond_codeGen(ctx, rhs), bb_then);
	pn->addIncoming(
		llvm::ConstantInt::get(
			llvm::Type::getInt1Ty(llvm::getGlobalContext()), 0), 
		bb_else);

	return pn;
}

llvm::Value* cond_bop_or_codeGen(const EvalCtx* ctx, const BOPOr* bop_or)
{
	const CondExpr		*lhs, *rhs;
	llvm::Value		*lhs_v;
	llvm::BasicBlock	*bb_then, *bb_else, *bb_merge;
	llvm::PHINode		*pn;
	llvm::Function		*f;

	f = builder->GetInsertBlock()->getParent();

	lhs = bop_or->getLHS();
	rhs = bop_or->getRHS();
	
	bb_then = llvm::BasicBlock::Create(
		llvm::getGlobalContext(), "then", f);
	bb_else = llvm::BasicBlock::Create(
		llvm::getGlobalContext(), "else");
	bb_merge = llvm::BasicBlock::Create(
		llvm::getGlobalContext(), "merge");

	lhs_v = cond_codeGen(ctx, lhs);
	/*
	 * if (lhs_v) {
	 * 	(bb_then)
	 * 	jmp merge
	 * } else {
	 * 	(bb_else)
	 * 	jmp merge
	 * }
	 * merge:
	 */
	builder->CreateCondBr(lhs_v, bb_then, bb_else);
	builder->SetInsertPoint(bb_then);
	builder->CreateBr(bb_merge);
	builder->SetInsertPoint(bb_else);
	builder->CreateBr(bb_merge);
	builder->SetInsertPoint(bb_merge);

	f->getBasicBlockList().push_back(bb_then);
	f->getBasicBlockList().push_back(bb_else);
	f->getBasicBlockList().push_back(bb_merge);

	/* short-circuit */
	pn->addIncoming(
		llvm::ConstantInt::get(
			llvm::Type::getInt1Ty(llvm::getGlobalContext()), 1), 
		bb_then);
	
	/* otherwise, then next stmt.. */
	pn->addIncoming(cond_codeGen(ctx, rhs), bb_else);

	return pn;

}



llvm::Value* cond_bop_codeGen(const EvalCtx* ctx, const BinBoolOp* bop)
{
	const BOPAnd*	bop_and;
	const BOPOr*	bop_or;

	const CondExpr*	cond_lhs;
	const CondExpr*	cond_rhs;

	assert (bop != NULL);

	cond_lhs = bop->getLHS();
	cond_rhs = bop->getRHS();

	bop_and = dynamic_cast<const BOPAnd*>(bop);
	if (bop_and != NULL) {
		/* if LHS evaluates to true, do RHS */
		return cond_bop_and_codeGen(ctx, bop_and);
	}
	
	bop_or = dynamic_cast<const BOPOr*>(bop);
	if (bop_or != NULL) {
		return cond_bop_or_codeGen(ctx, bop_or);
	}

	/* should not happen */
	assert (0 == 1);
	return NULL;
}

llvm::Value* cond_codeGen(const EvalCtx* ctx, const CondExpr* cond)
{
	const CmpOp	*cmpop;
	const BinBoolOp	*bop;
	const FuncCond	*fcond;

	cmpop = dynamic_cast<const CmpOp*>(cond);
	if (cmpop != NULL) {
		return cond_cmpop_codeGen(ctx, cmpop);
	}

	bop = dynamic_cast<const BinBoolOp*>(cond);
	if (bop != NULL) 
		return cond_bop_codeGen(ctx, bop);

	fcond = dynamic_cast<const FuncCond*>(cond);
	assert (fcond != NULL);

	return evalAndGen(*ctx, fcond->getFC());
}

