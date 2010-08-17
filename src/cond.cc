#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/IRBuilder.h>
#include <assert.h>

#include "expr.h"
#include "cond.h"
#include "code_builder.h"

extern CodeBuilder*		code_builder;


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

	return code_builder->getBuilder()->CreateICmp(pred, lhs, rhs);
}

llvm::Value* cond_bop_and_codeGen(const EvalCtx* ctx, const BOPAnd* bop_and)
{
	const CondExpr		*lhs, *rhs;
	llvm::Value		*lhs_v, *rhs_v;
	llvm::BasicBlock	*bb_then, *bb_else, *bb_merge, *bb_origin;
	llvm::PHINode		*pn;
	llvm::Function		*f;
	llvm::IRBuilder<>	*builder;
	llvm::LLVMContext	&gctx(llvm::getGlobalContext());

	builder = code_builder->getBuilder();
	bb_origin = builder->GetInsertBlock();
	f = bb_origin->getParent();

	lhs = bop_and->getLHS();
	rhs = bop_and->getRHS();
	
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

	bb_then = llvm::BasicBlock::Create(gctx, "and_then", f);
	bb_else = llvm::BasicBlock::Create(gctx, "and_else", f);
	bb_merge = llvm::BasicBlock::Create(gctx, "and_merge", f);


	builder->SetInsertPoint(bb_origin);
	lhs_v = cond_codeGen(ctx, lhs);
	builder->CreateCondBr(lhs_v, bb_then, bb_else);

	builder->SetInsertPoint(bb_then);
	rhs_v = cond_codeGen(ctx, rhs);
	builder->CreateBr(bb_merge);

	builder->SetInsertPoint(bb_else);
	builder->CreateBr(bb_merge);

	builder->SetInsertPoint(bb_merge);

	pn = builder->CreatePHI(llvm::Type::getInt1Ty(gctx), "iftmp");

	/* lhs_v true, eval rhs_v */
	pn->addIncoming(rhs_v, bb_then);

	/* lhs_v false, don't bother evaluating rhs_v */
	pn->addIncoming(
		llvm::ConstantInt::get(llvm::Type::getInt1Ty(gctx), 0), 
		bb_else);

	return pn;
}

llvm::Value* cond_bop_or_codeGen(const EvalCtx* ctx, const BOPOr* bop_or)
{
	const CondExpr		*lhs, *rhs;
	llvm::Value		*lhs_v, *rhs_v;
	llvm::BasicBlock	*bb_then, *bb_else, *bb_merge, *bb_origin;
	llvm::PHINode		*pn;
	llvm::Function		*f;
	llvm::IRBuilder<>	*builder;
	llvm::LLVMContext	&gctx(llvm::getGlobalContext());
	
	builder = code_builder->getBuilder();
	bb_origin = builder->GetInsertBlock();

	f = bb_origin->getParent();

	lhs = bop_or->getLHS();
	rhs = bop_or->getRHS();
	
	bb_then = llvm::BasicBlock::Create(gctx, "or_then", f);
	bb_else = llvm::BasicBlock::Create(gctx, "or_else", f);
	bb_merge = llvm::BasicBlock::Create(gctx, "or_merge", f);

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
	builder->SetInsertPoint(bb_origin);
	lhs_v = cond_codeGen(ctx, lhs);
	builder->CreateCondBr(lhs_v, bb_then, bb_else);

	builder->SetInsertPoint(bb_then);
	builder->CreateBr(bb_merge);

	builder->SetInsertPoint(bb_else);
	rhs_v = cond_codeGen(ctx, rhs);

	builder->CreateBr(bb_merge);
	builder->SetInsertPoint(bb_merge);

	/* short-circuit */
	pn = builder->CreatePHI(llvm::Type::getInt1Ty(gctx), "or_iftmp");
	pn->addIncoming(
		llvm::ConstantInt::get(llvm::Type::getInt1Ty(gctx), 1), 
		bb_then);
	
	/* otherwise, try the next stmt, if might be true.. */
	pn->addIncoming(rhs_v, bb_else);


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
		/* if LHS evaluates to true, go ahead and do RHS */
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

/**
 * generates boolean value for given condition
 */
llvm::Value* cond_codeGen(const EvalCtx* ctx, const CondExpr* cond)
{
	const CmpOp	*cmpop;
	const CondNot	*cnot;
	const BinBoolOp	*bop;
	const FuncCond	*fcond;
	llvm::Value	*ret;

	cnot = dynamic_cast<const CondNot*>(cond);
	if (cnot != NULL) {
		const CondExpr	*inner_cond;

		inner_cond = cnot->getExpr();
		ret = cond_codeGen(ctx, inner_cond);
		if (ret == NULL)
			return ret;
		return code_builder->getBuilder()->CreateNot(ret);
	}
	

	if ((cmpop = dynamic_cast<const CmpOp*>(cond)) != NULL) {
		ret = cond_cmpop_codeGen(ctx, cmpop);
	} else if ((bop = dynamic_cast<const BinBoolOp*>(cond)) != NULL) {
		ret = cond_bop_codeGen(ctx, bop);
	} else if ((fcond = dynamic_cast<const FuncCond*>(cond)) != NULL) {
		ret = evalAndGen(*ctx, fcond->getFC());
	} else {
		assert (0 == 1);
	}

	return ret;
}

