#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/IRBuilder.h>

#include "cond.h"
#include "func.h"
#include "eval.h"
#include "evalctx.h"
#include "code_builder.h"

using namespace std;
using namespace llvm;

extern CodeBuilder	*code_builder;
extern ctype_map	ctypes_map;
extern type_map		types_map;
extern const FuncBlock	*gen_func_block;
extern const Func	*gen_func;

static bool gen_func_code_args(
	const Func* f, 
	vector<const llvm::Type*>& llvm_args);

Func* FuncStmt::getFunc(void) const
{
	if (f_owner == NULL)
		return owner->getFunc();

	return f_owner;
}

void Func::genLoadArgs(void) const
{
	Function			*f;
	Function::arg_iterator		ai;
	unsigned int			arg_c;

	arg_c = args->size();

	f = getFunction();
	ai = f->arg_begin();
	IRBuilder<>			tmpB(
		&f->getEntryBlock(),
		f->getEntryBlock().begin());

	for (unsigned int i = 0; i < arg_c; i++, ai++) {
		AllocaInst		*allocai;
		const llvm::Type	*t;
		const ::Type*		user_type;
		string			arg_name, type_name;
		bool			is_user_type;

		type_name = (args->get(i).first)->getName();
		arg_name = (args->get(i).second)->getName();

		is_user_type = (types_map.count(type_name) > 0);
		if (is_user_type) {
			user_type = types_map[type_name];
			t = code_builder->getClosureTyPtr();
		} else {
			user_type = NULL;
			t = llvm::Type::getInt64Ty(llvm::getGlobalContext());
		}

		allocai = tmpB.CreateAlloca(t, 0, arg_name);
		block->addVar(user_type, arg_name, allocai);
		code_builder->getBuilder()->CreateStore(ai, allocai);
	}

	/* handle return value if passing type */
	if (f->getReturnType() == tmpB.getVoidTy()) {
		AllocaInst		*allocai;
		const llvm::Type	*t;

		t = code_builder->getClosureTyPtr();

		allocai = tmpB.CreateAlloca(t, 0, "__ret_closure");
		block->addVar(types_map[getRet()], "__ret_closure", allocai);
		code_builder->getBuilder()->CreateStore(ai, allocai);
	}
}

Value* FuncDecl::codeGen(void) const
{
	FuncBlock		*scope;
	Function		*f = getFunc()->getFunction();
	AllocaInst		*ai;
	const ::Type		*user_type;
	IRBuilder<>	tmpB(&f->getEntryBlock(), f->getEntryBlock().begin());

	/* XXX no support for arrays yet */
	assert (array == NULL);

	if (types_map.count(type->getName()) != 0) {
		user_type = types_map[type->getName()];
		ai = code_builder->createTmpClosure(
			user_type, scalar->getName());
	} else if (ctypes_map.count(type->getName()) != 0) {
		const llvm::Type	*t;
		t = llvm::Type::getInt64Ty(llvm::getGlobalContext());
		user_type = NULL;
		ai = tmpB.CreateAlloca(t, 0, scalar->getName());
	} else {
		cerr << getLineNo() << ": could not resolve type for "
			<< type->getName();
		return NULL;
	}

	scope = getOwner();
	if (scope->addVar(user_type, scalar->getName(), ai) == false) {
		cerr << "Already added variable " << scalar->getName() << endl;
		return NULL;
	}

	return NULL;
}

Value* FuncAssign::codeGen(void) const
{
	Value			*e_v;
	llvm::AllocaInst*	var_loc;
	EvalCtx			ectx(getOwner());
	IRBuilder<>		*builder;

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
	assert (var_loc != NULL);

	builder = code_builder->getBuilder();
	if (e_v->getType() == code_builder->getClosureTy()) {
		cerr << "AAAAAAAAA" << endl;
		code_builder->copyClosure(
			getOwner()->getVarType(scalar->getName()),
			builder->CreateGEP(e_v,
				llvm::ConstantInt::get(
					llvm::getGlobalContext(),
					llvm::APInt(32, 0))),
			builder->CreateLoad(var_loc));
		cerr << "BBBBBB" << endl;
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
			types_map[getFunc()->getRet()],
			e_v, tp_elem_ptr);

		builder->CreateRetVoid();

		return NULL;
	}

	/* return result */
	builder->CreateRet(e_v);

	return e_v;
}

Value* FuncCondStmt::codeGen(void) const
{
	Function	*f;
	BasicBlock	*bb_then, *bb_else, *bb_merge, *bb_origin, *bb_cur;
	Value		*cond_v;
	IRBuilder<>	*builder;
	EvalCtx		ectx(getOwner());

	builder = code_builder->getBuilder();
	f = getFunction();

	bb_origin = builder->GetInsertBlock();

	bb_then = BasicBlock::Create(getGlobalContext(), "func_then", f);
	if (is_false != NULL)
		bb_else = BasicBlock::Create(getGlobalContext(), "func_else", f);
	else
		bb_else = NULL;

	bb_merge = BasicBlock::Create(getGlobalContext(), "func_merge", f);

	builder->SetInsertPoint(bb_origin);
	cond_v = cond_codeGen(&ectx, cond);
	if (cond_v == NULL) {
		cerr << getLineNo() << ": could not gen condition" << endl;
		return NULL;
	}
	builder->CreateCondBr(cond_v, bb_then, (is_false) ? bb_else : bb_merge);


	builder->SetInsertPoint(bb_then);
	is_true->codeGen();
	if (bb_then->empty() || bb_then->back().isTerminator() == false)
		builder->CreateBr(bb_merge);

	if (bb_else != NULL) {
		builder->SetInsertPoint(bb_else);
		is_false->codeGen();
		if (bb_else->empty() || bb_else->back().isTerminator() == false)
			builder->CreateBr(bb_merge);
	}

	bb_cur = builder->GetInsertBlock();
	if (bb_cur->empty())
		builder->CreateBr(bb_merge);

	builder->SetInsertPoint(bb_merge);

	return cond_v;
}

Value* FuncBlock::codeGen(void) const
{
	for (const_iterator it = begin(); it != end(); it++) {
		FuncStmt	*fstmt = *it;
		gen_func_block = this;
		fstmt->codeGen();
	}

	return NULL;
}

AllocaInst* FuncBlock::getVar(const std::string& s) const
{
	funcvar_map::const_iterator	it;

	it = vars.find(s);
	if (it == vars.end()) {
		if (getOwner() == NULL)
			return NULL;

		return getOwner()->getVar(s);
	}

	return ((*it).second).fv_ai;
}

const ::Type* FuncBlock::getVarType(const std::string& s) const
{
	funcvar_map::const_iterator	it;

	it = vars.find(s);
	if (it == vars.end()) {
		if (getOwner() == NULL)
			return NULL;

		return getOwner()->getVarType(s);
	}

	return ((*it).second).fv_t;
}

bool FuncBlock::addVar(
	const ::Type* t,
	const std::string& name, llvm::AllocaInst* ai)
{
	struct FuncVar	fv;

	if (vars.count(name) != 0)
		return false;

	fv.fv_ai = ai;
	fv.fv_t = t;

	vars[name] = fv;
	return true;
}

Function* Func::getFunction(void) const
{
	return code_builder->getModule()->getFunction(getName());
}

Function* FuncStmt::getFunction() const
{
	return getFunc()->getFunction();
}

void Func::genProto(void) const
{
	vector<const llvm::Type*>	f_args;
	llvm::Function			*llvm_f;
	llvm::FunctionType		*llvm_ft;
	const llvm::Type		*t_ret;
	bool				is_user_type;

	if (types_map.count(getRet()) != 0) {
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
		

	if (gen_func_code_args(this, f_args) == false) {
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

static bool gen_func_code_args(
	const Func* f, vector<const llvm::Type*>& llvm_args)
{
	const ArgsList	*args;
	bool		is_ret_user_type;

	is_ret_user_type = (types_map.count(f->getRet()) != 0);

	args = f->getArgs();
	assert (args != NULL);

	llvm_args.clear();

	for (unsigned int i = 0; i < args->size(); i++) {
		string			cur_type;
		bool			is_user_type;
		const llvm::Type	*t;

		cur_type = (((args->get(i)).first)->getName());
		if (types_map.count(cur_type) != 0) {
			is_user_type = true;
		} else if (ctypes_map.count(cur_type) != 0) {
			is_user_type = false;
		} else {
			cerr << f->getName() << 
				": Could not resolve argument type for \"" <<
				cur_type << '"' << endl;
			return false;
		}

		if (is_user_type) {
			t = code_builder->getClosureTyPtr();
		} else {
			t = llvm::Type::getInt64Ty(llvm::getGlobalContext());
		}

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


