#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/IRBuilder.h>
#include <assert.h>

#include "expr.h"
#include "eval.h"
#include "code_builder.h"

extern CodeBuilder*		code_builder;

using namespace std;

llvm::Value* CmpOp::codeGen(const EvalCtx* ctx) const
{
	llvm::Value			*v_lhs, *v_rhs;
	llvm::CmpInst::Predicate	pred;

	v_lhs = (ctx) ? evalAndGen(*ctx, getLHS()) : getLHS()->codeGen();
	v_rhs = (ctx) ? evalAndGen(*ctx, getRHS()) : getRHS()->codeGen();

	switch (getOp()) {
	case CmpOp::EQ: pred = llvm::CmpInst::ICMP_EQ; break;
	case CmpOp::NE: pred = llvm::CmpInst::ICMP_NE; break;
	case CmpOp::LE: pred = llvm::CmpInst::ICMP_ULE; break;
	case CmpOp::LT: pred = llvm::CmpInst::ICMP_ULT; break;
	case CmpOp::GT: pred = llvm::CmpInst::ICMP_UGT; break;
	case CmpOp::GE: pred = llvm::CmpInst::ICMP_UGE; break;
	default:
		pred = llvm::CmpInst::ICMP_EQ;
		assert (0 == 1 && "Bad CmpOP");
	}
	return code_builder->getBuilder()->CreateICmp(pred, v_lhs, v_rhs);
}

llvm::Value* BOPAnd::codeGen(const EvalCtx* ctx) const
{
	llvm::Value		*lhs_v, *rhs_v;
	llvm::BasicBlock	*bb_then, *bb_else, *bb_merge, *bb_origin;
	llvm::BasicBlock	*bb_rhs_merge;
	llvm::PHINode		*pn;
	llvm::Function		*f;
	llvm::IRBuilder<>	*builder;
	llvm::LLVMContext	&gctx(llvm::getGlobalContext());

	builder = code_builder->getBuilder();
	bb_origin = builder->GetInsertBlock();
	f = bb_origin->getParent();

	bb_then = llvm::BasicBlock::Create(gctx, "and_then", f);
	bb_else = llvm::BasicBlock::Create(gctx, "and_else", f);
	bb_merge = llvm::BasicBlock::Create(gctx, "and_merge", f);

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
	lhs_v = cond_lhs->codeGen(ctx);
	builder->CreateCondBr(lhs_v, bb_then, bb_else);

	builder->SetInsertPoint(bb_then);
	rhs_v = cond_rhs->codeGen(ctx);
	bb_rhs_merge = builder->GetInsertBlock();
	builder->CreateBr(bb_merge);

	builder->SetInsertPoint(bb_else);
	builder->CreateBr(bb_merge);

	builder->SetInsertPoint(bb_merge);

	pn = builder->CreatePHI(llvm::Type::getInt1Ty(gctx), 2, "and_iftmp");

	/* lhs_v true, eval rhs_v */
	pn->addIncoming(rhs_v, bb_rhs_merge);

	/* lhs_v false, don't bother evaluating rhs_v */
	pn->addIncoming(
		llvm::ConstantInt::get(llvm::Type::getInt1Ty(gctx), 0),
		bb_else);

	return pn;
}

llvm::Value* BOPOr::codeGen(const EvalCtx* ctx) const
{
	llvm::Value		*lhs_v, *rhs_v;
	llvm::BasicBlock	*bb_then, *bb_else, *bb_merge, *bb_origin;
	llvm::BasicBlock	*bb_rhs_merge;
	llvm::PHINode		*pn;
	llvm::Function		*f;
	llvm::IRBuilder<>	*builder;
	llvm::LLVMContext	&gctx(llvm::getGlobalContext());

	builder = code_builder->getBuilder();
	bb_origin = builder->GetInsertBlock();
	f = bb_origin->getParent();

	bb_then = llvm::BasicBlock::Create(gctx, "or_then", f);
	bb_else = llvm::BasicBlock::Create(gctx, "or_else", f);
	bb_merge = llvm::BasicBlock::Create(gctx, "or_merge", f);

	/* see BOPAnd for explanation of jmps and merging */
	builder->SetInsertPoint(bb_origin);
	lhs_v = cond_lhs->codeGen(ctx);
	builder->CreateCondBr(lhs_v, bb_then, bb_else);

	builder->SetInsertPoint(bb_then);
	builder->CreateBr(bb_merge);

	builder->SetInsertPoint(bb_else);
	rhs_v = cond_rhs->codeGen(ctx);
	bb_rhs_merge = builder->GetInsertBlock();
	builder->CreateBr(bb_merge);

	builder->SetInsertPoint(bb_merge);

	/* short-circuit */
	pn = builder->CreatePHI(llvm::Type::getInt1Ty(gctx), 2, "or_iftmp");
	pn->addIncoming(
		llvm::ConstantInt::get(llvm::Type::getInt1Ty(gctx), 1),
		bb_then);

	/* otherwise, try the next stmt, if might be true.. */
	pn->addIncoming(rhs_v, bb_rhs_merge);

	return pn;
}

llvm::Value* CondNot::codeGen(const EvalCtx* ctx) const
{
	llvm::Value	*in_v = cexpr->codeGen(ctx);
	if (in_v == NULL) return NULL;
	return code_builder->getBuilder()->CreateNot(in_v);
}

llvm::Value* FuncCond::codeGen(const EvalCtx* ctx) const
{
	if (ctx == NULL) return getFC()->codeGen();
	return evalAndGen(*ctx, getFC());
}
