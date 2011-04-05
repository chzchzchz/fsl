#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/IRBuilder.h>

#include "func.h"
#include "eval.h"
#include "evalctx.h"
#include "code_builder.h"
#include "runtime_interface.h"

using namespace std;
using namespace llvm;

extern CodeBuilder	*code_builder;
extern RTInterface	rt_glue;
extern ctype_map	ctypes_map;
extern type_map		types_map;
extern const FuncBlock	*gen_func_block;
extern const Func	*gen_func;

Func* FuncStmt::getFunc(void) const
{
	return (f_owner) ? f_owner : owner->getFunc();
}

void Func::genLoadArgs(void) const
{
	Function			*f = getFunction();
	Function::arg_iterator		ai = f->arg_begin();
	IRBuilder<>			tmpB(
		&f->getEntryBlock(),
		f->getEntryBlock().begin());

	block->vscope.loadArgs(ai, &tmpB, args);

	/* handle return value if passing type */
	if (f->getReturnType() == tmpB.getVoidTy()) {
		block->vscope.createTmpClosurePtr(
			getRetType(), "__ret_closure", ai, tmpB);
	}
}

Value* FuncDecl::codeGen(void) const
{
	FuncBlock		*scope = getOwner();
	Function		*f = getFunc()->getFunction();
	const ::Type		*user_type;
	IRBuilder<>	tmpB(&f->getEntryBlock(), f->getEntryBlock().begin());

	/* XXX no support for arrays yet */
	assert (array == NULL);

	if (scope->getVar(scalar->getName()) != NULL) {
		cerr << "Already added variable " << scalar->getName() << endl;
		return NULL;
	}

	if (types_map.count(type->getName()) != 0) {
		user_type = types_map[type->getName()];
		scope->vscope.createTmpClosure(user_type, scalar->getName());
	} else if (ctypes_map.count(type->getName()) != 0) {
		scope->vscope.createTmpI64(scalar->getName());
	} else {
		cerr << getLineNo() << ": could not resolve type for "
			<< type->getName();
		return NULL;
	}

	return NULL;
}

bool FuncAssign::genDebugAssign(void) const
{
	EvalCtx			ectx(getOwner());

	/* send value to runtime for debugging purposes */
	if (scalar->getName() == "DEBUG_WRITE") {
		Expr	*e;
		e = rt_glue.getDebugCall(eval(ectx, expr));
		evalAndGen(ectx, e);
		delete e;
		return true;
	}

	/* send type data to runtime for debugging purposes */
	if (scalar->getName() == "DEBUG_WRITE_TYPE") {
		Expr		*e;
		const ::Type	*clo_type;

		clo_type = ectx.getType(expr);
		if (clo_type == NULL) {
			cerr << "Oops: Could not resolve type for expression: ";
			expr->print(cerr);
			cerr << endl;
			exit(-1);
			return NULL;
		}

		e = rt_glue.getDebugCallTypeInstance(
			clo_type,
			eval(ectx, expr));

		evalAndGen(ectx, e);
		delete e;
		return true;
	}

	return false;
}

Value* FuncAssign::codeGen(void) const
{
	Value			*e_v;
	llvm::AllocaInst*	var_loc;
	EvalCtx			ectx(getOwner());
	IRBuilder<>		*builder;

	/* send data to runtime for debugging purposes */
	/* awful awful syntax abuse */
	if (genDebugAssign()) return NULL;

	e_v = evalAndGen(ectx, expr);
	if (e_v == NULL) {
		cerr << getLineNo() << ": Could not eval ";
		expr->print(cerr);
		cerr << endl;
		return NULL;

	}

	/* XXX no support for arrays just yet */
	assert (array == NULL && "Can't assign to arrays yet");

	var_loc = getOwner()->getVar(scalar->getName());
	assert (var_loc != NULL && "Expected variable name to assign to");

	builder = code_builder->getBuilder();
	if (e_v->getType() == code_builder->getClosureTy()) {
		code_builder->copyClosure(
			getOwner()->getVarType(scalar->getName()),
			builder->CreateGEP(e_v,
				llvm::ConstantInt::get(
					llvm::getGlobalContext(),
					llvm::APInt(32, 0))),
			builder->CreateLoad(var_loc));
	} else {
		builder->CreateStore(e_v /* src */, var_loc /* dst */);
	}

	return e_v;
}

Value* FuncRet::codeGen(void) const
{
	Value			*e_v;
	IRBuilder<>		*builder;
	BasicBlock		*cur_bb;
	const Function		*f;
	const llvm::Type	*t;
	EvalCtx			ectx(getOwner());

	e_v = evalAndGen(ectx, expr);
	if (e_v == NULL) {
		cerr << getLineNo() << ": FuncRet could not eval ";
		expr->print(cerr);
		cerr << endl;
	}

	builder = code_builder->getBuilder();
	cur_bb = builder->GetInsertBlock();

	/* if return type is boolean, truncate 64-bit return value */
	f = cur_bb->getParent();
	t = f->getReturnType();
	if (t == llvm::Type::getInt1Ty(llvm::getGlobalContext())) {
		e_v = builder->CreateTrunc(e_v, t);
	} else if (t == builder->getVoidTy()) {
		/* copy result into return value */
		llvm::Value			*tp_elem_ptr;
		AllocaInst			*allocai;

		allocai = getOwner()->getVar("__ret_closure");
		assert (allocai != NULL);
		tp_elem_ptr = builder->CreateLoad(allocai);

		code_builder->copyClosure(
			getFunc()->getRetType(), e_v, tp_elem_ptr);
		builder->CreateRetVoid();

		return NULL;
	}

	/* return result */
	builder->CreateRet(e_v);

	return e_v;
}

Value* FuncCondStmt::codeGen(void) const
{
	Function	*f = getFunction();
	IRBuilder<>	*builder = code_builder->getBuilder();

	BasicBlock	*bb_then, *bb_else, *bb_merge, *bb_origin, *bb_cur;
	Value		*cond_v;
	EvalCtx		ectx(getOwner());

	/* setup allocate BBs */
	bb_origin = builder->GetInsertBlock();
	bb_then = BasicBlock::Create(getGlobalContext(), "func_then", f);
	if (is_false != NULL)
		bb_else = BasicBlock::Create(getGlobalContext(), "func_else", f);
	else
		bb_else = NULL;
	bb_merge = BasicBlock::Create(getGlobalContext(), "func_merge", f);

	/* generate conditional jump */
	builder->SetInsertPoint(bb_origin);
	cond_v = cond->codeGen(&ectx);
	if (cond_v == NULL) {
		cerr << getLineNo() << ": could not gen condition" << endl;
		return NULL;
	}
	builder->CreateCondBr(cond_v, bb_then, (is_false) ? bb_else : bb_merge);

	/* create true condition */
	builder->SetInsertPoint(bb_then);
	is_true->codeGen();
	/* if we didn't jump out of scope, go to merged BB */
	if (bb_then->empty()|| bb_then->back().isTerminator() == false)
		builder->CreateBr(bb_merge);
	/* patch up */
	bb_cur = builder->GetInsertBlock();
	if (bb_cur->empty())
		builder->CreateBr(bb_merge);


	/* create false condition */
	if (bb_else != NULL) {
		builder->SetInsertPoint(bb_else);
		is_false->codeGen();
		if (bb_else->empty() || bb_else->back().isTerminator() == false)
			builder->CreateBr(bb_merge);

		/* patch up */
		bb_cur = builder->GetInsertBlock();
		if (bb_cur->empty())
			builder->CreateBr(bb_merge);
	}

	builder->SetInsertPoint(bb_merge);

	return cond_v;
}

Value* FuncWhileStmt::codeGen(void) const
{
	Function	*f = getFunction();
	IRBuilder<>	*builder = code_builder->getBuilder();

	BasicBlock	*bb_begin, *bb_cond, *bb_end, *bb_origin;
	BasicBlock	*bb_after_body;
	Value		*cond_v;
	EvalCtx		ectx(getOwner());

	bb_origin = builder->GetInsertBlock();
	bb_cond = BasicBlock::Create(getGlobalContext(), "while_cond", f);
	bb_begin = BasicBlock::Create(getGlobalContext(), "while_begin", f);
	bb_end = BasicBlock::Create(getGlobalContext(), "while_end", f);

	builder->SetInsertPoint(bb_origin);
	builder->CreateBr(bb_cond);

	/* while condition */
	builder->SetInsertPoint(bb_cond);
	cond_v = cond->codeGen(&ectx);
	if (cond_v == NULL) {
		cerr << getLineNo() << ": could not gen condition" << endl;
		return NULL;
	}
	builder->CreateCondBr(cond_v, bb_begin, bb_end);

	/* while body */
	builder->SetInsertPoint(bb_begin);
	stmt->codeGen();

	bb_after_body = builder->GetInsertBlock();
	if (bb_after_body->empty() ||
	    bb_after_body->back().isTerminator() == false) {
		/* body doesn't have a branch or return stmt.. loop. */
		builder->CreateBr(bb_cond);
	}

	assert (bb_begin->empty() == false && "No body in while?");

	/* generate code for outside of loop body */
	builder->SetInsertPoint(bb_end);

	return cond_v;
}

Value* FuncBlock::codeGen(void) const
{
	for (const_iterator it = begin(); it != end(); it++) {
		gen_func_block = this;
		(*it)->codeGen();
	}

	return NULL;
}

AllocaInst* FuncBlock::getVar(const std::string& s) const
{
	FuncBlock	*owner;
	AllocaInst	*ret;

	if ((ret = vscope.getVar(s)) != NULL) return ret;

	if ((owner = getOwner()) == NULL) return NULL;
	return owner->getVar(s);
}

const ::Type* FuncBlock::getVarType(const std::string& s) const
{
	FuncBlock	*owner;
	const ::Type	*ret;

	if ((ret = vscope.getVarType(s)) != NULL) return ret;

	if ((owner = getOwner()) == NULL) return NULL;
	return owner->getVarType(s);
}

Function* Func::getFunction(void) const
{
	return code_builder->getModule()->getFunction(getName());
}

Function* FuncStmt::getFunction() const { return getFunc()->getFunction(); }

void Func::genProto(void) const
{
	vector<const llvm::Type*>	f_args;
	llvm::Function			*llvm_f;
	llvm::FunctionType		*llvm_ft;
	const llvm::Type		*t_ret;
	bool				is_user_type;

	if (getRetType() != NULL) {
		is_user_type = true;
	} else if (ctypes_map.count(getRet()) != 0) {
		is_user_type = false;
	} else {
		cerr	<< "Bad return type (" << getRet() << ") for "
			<< getName() << endl;
		return;
	}

	/* XXX need to do this better.. */
	if (getRet() == "bool") {
		t_ret = llvm::Type::getInt1Ty(llvm::getGlobalContext());
	} else if (is_user_type == false) {
		t_ret = llvm::Type::getInt64Ty(llvm::getGlobalContext());
	} else {
		/* we'll tack on a parameter at the end that is a pointer
		 * to a typepass struct, which will be what we write our
		 * return data to (return will act as an assignment) */
		t_ret = llvm::Type::getVoidTy(llvm::getGlobalContext());
	}

	if (genFuncCodeArgs(f_args) == false) {
		cerr << "Bailing on generating " << getName() << endl;
		return;
	}

	/* finally, create the proto */
	llvm_ft = llvm::FunctionType::get(t_ret, f_args, false);

	llvm_f = llvm::Function::Create(
		llvm_ft,
		llvm::Function::ExternalLinkage,
		getName(),
		code_builder->getModule());
	if (llvm_f->getName() != getName()) {
		cerr << getName() << " already declared!" << endl;
	}
}

bool Func::genFuncCodeArgs(vector<const llvm::Type*>& llvm_args) const
{
	const ArgsList	*args = getArgs();
	bool		is_ret_user_type;

	assert (args != NULL);

	is_ret_user_type = (getRetType() != NULL);
	llvm_args.clear();

	for (unsigned int i = 0; i < args->size(); i++) {
		string			cur_type;
		bool			is_user_type;
		const llvm::Type	*t;

		cur_type = (((args->get(i)).first)->getName());
		if (types_map.count(cur_type) != 0) is_user_type = true;
		else if (ctypes_map.count(cur_type) != 0) is_user_type = false;
		else {
			cerr << getName() <<
				": Could not resolve argument type for \"" <<
				cur_type << '"' << endl;
			return false;
		}

		if (is_user_type)
			t = code_builder->getClosureTyPtr();
		else
			t = llvm::Type::getInt64Ty(llvm::getGlobalContext());

		llvm_args.push_back(t);
	}

	if (is_ret_user_type) {
		/* hidden return value */
		llvm_args.push_back(code_builder->getClosureTyPtr());
	}

	return true;
}

void Func::genCode(void) const
{
	llvm::Function			*llvm_f;
	llvm::BasicBlock		*f_bb;

	cerr << "Generating code for " << name->getName() << endl;

	llvm_f = code_builder->getModule()->getFunction(getName());
	if (llvm_f == NULL) {
		cerr	<< "Could not find function prototype for "
			<< getName() << endl;
		return;
	}

	f_bb = llvm::BasicBlock::Create(
		llvm::getGlobalContext(), "entry", llvm_f);

	code_builder->getBuilder()->SetInsertPoint(f_bb);

	/* set gen_func so that expr id resolution will get the right
	 * alloca variable */
	gen_func = this;

	/* create arguments */
	genLoadArgs();

	/* gen rest of code */
	block->codeGen();

	gen_func = NULL;
	gen_func_block = NULL;
}

const ::Type* Func::getRetType(void) const
{
	if (types_map.count(ret_type->getName()) == 0) return NULL;
	return types_map[ret_type->getName()];
}
