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
		string			arg_name, type_name;
		bool			is_user_type;

		type_name = (args->get(i).first)->getName();
		arg_name = (args->get(i).second)->getName();

		is_user_type = (types_map.count(type_name) > 0);
		if (is_user_type) {
			t = code_builder->getTypePassStruct();
		} else {
			t = llvm::Type::getInt64Ty(llvm::getGlobalContext());
		}

		allocai = tmpB.CreateAlloca(t, 0, arg_name);
		block->addVar(arg_name, allocai);
		code_builder->getBuilder()->CreateStore(ai, allocai);
	}

	/* handle return value if passing type */
	if (f->getReturnType() == tmpB.getVoidTy()) {
		AllocaInst		*allocai;
		const llvm::Type	*t;

		t = code_builder->getTypePassStructPtr();

		allocai = tmpB.CreateAlloca(t, 0, "__ret_typepass");
		block->addVar("__ret_typepass", allocai);
		code_builder->getBuilder()->CreateStore(ai, allocai);
	}
}

Value* FuncDecl::codeGen(const EvalCtx* ectx) const
{
	FuncBlock		*scope;
	Function		*f = getFunc()->getFunction();
	const llvm::Type	*t;
	AllocaInst		*ai;
	IRBuilder<>	tmpB(&f->getEntryBlock(), f->getEntryBlock().begin());

	/* XXX no support for arrays yet */
	assert (array == NULL);

	if (ctypes_map.count(type->getName()) == 0) {
		cerr << getLineNo() << ": could not resolve type for "
			<< type->getName();
		return NULL;
	} else {
		/* lazy about widths right now */
		t = llvm::Type::getInt64Ty(llvm::getGlobalContext());
	}

	ai = tmpB.CreateAlloca(t, 0, scalar->getName());

	scope = getOwner();
	if (scope->addVar(scalar->getName(), ai) == false) {
		cerr << "Already added variable " << scalar->getName() << endl;
		return NULL;
	}

	return NULL;
}

Value* FuncAssign::codeGen(const EvalCtx* ectx) const
{
	Value			*e_v;
	llvm::AllocaInst*	var_loc;

	e_v = evalAndGen(*ectx, expr);
	if (e_v == NULL) {
		cerr << getLineNo() << ": Could not eval ";
		expr->print(cerr);
		cerr << endl;
		return NULL;

	}

	/* XXX no support for arrays just yet */
	assert (array == NULL && "Can't assign to arrays yet");

	var_loc = getOwner()->getVar(scalar->getName());
	code_builder->getBuilder()->CreateStore(e_v, var_loc);

	return e_v;
}

Value* FuncRet::codeGen(const EvalCtx* ectx) const
{
	Value			*e_v;
	IRBuilder<>		*builder;
	BasicBlock		*cur_bb;
	const Function		*f;
	const llvm::Type	*t;

	e_v = evalAndGen(*ectx, expr);
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

		allocai = getOwner()->getVar("__ret_typepass");
		assert (allocai != NULL);
		tp_elem_ptr = builder->CreateLoad(allocai);

		code_builder->copyTypePassStruct(
			types_map[getFunc()->getRet()],
			e_v, tp_elem_ptr);

		builder->CreateRetVoid();

		return NULL;
	}

	/* return result */
	builder->CreateRet(e_v);

	return e_v;
}

Value* FuncCondStmt::codeGen(const EvalCtx* ectx) const
{
	Function	*f;
	BasicBlock	*bb_then, *bb_else, *bb_merge, *bb_origin, *bb_cur;
	Value		*cond_v;
	IRBuilder<>	*builder;

	builder = code_builder->getBuilder();
	f = getFunction();

	bb_origin = builder->GetInsertBlock();

	bb_then = BasicBlock::Create(getGlobalContext(), "func_then", f);
	if (is_false != NULL)
		bb_else = BasicBlock::Create(getGlobalContext(), "func_else", f);
	bb_merge = BasicBlock::Create(getGlobalContext(), "func_merge", f);

	builder->SetInsertPoint(bb_origin);
	cond_v = cond_codeGen(ectx, cond);
	if (cond_v == NULL) {
		cerr << getLineNo() << ": could not gen condition" << endl;
		return NULL;
	}
	builder->CreateCondBr(cond_v, bb_then, (is_false) ? bb_else : bb_merge);


	builder->SetInsertPoint(bb_then);
	is_true->codeGen(ectx);
	if (bb_then->empty() || bb_then->back().isTerminator() == false)
		builder->CreateBr(bb_merge);

	if (is_false) {
		builder->SetInsertPoint(bb_else);
		is_false->codeGen(ectx);
		if (bb_else->empty() || bb_else->back().isTerminator() == false)
			builder->CreateBr(bb_merge);
	}

	bb_cur = builder->GetInsertBlock();
	if (bb_cur->empty())
		builder->CreateBr(bb_merge);

	builder->SetInsertPoint(bb_merge);

	return cond_v;
}

Value* FuncBlock::codeGen(const EvalCtx* ectx) const
{
	for (const_iterator it = begin(); it != end(); it++) {
		FuncStmt	*fstmt = *it;
		gen_func_block = this;
		fstmt->codeGen(ectx);
	}

	return NULL;
}

AllocaInst* FuncBlock::getVar(const std::string& s) const
{
	map<string, AllocaInst*>::const_iterator	it;

	it = vars.find(s);
	if (it == vars.end()) {
		if (getOwner() == NULL)
			return NULL;

		return getOwner()->getVar(s);
	}

	return (*it).second;
}

bool FuncBlock::addVar(const std::string& name, llvm::AllocaInst* ai)
{
	if (vars.count(name) != 0)
		return false;

	vars[name] = ai;
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
	llvm::Type			*ret_type;
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
			t = code_builder->getTypePassStruct();
		} else {
			t = llvm::Type::getInt64Ty(llvm::getGlobalContext());
		}

		llvm_args.push_back(t);
	}

	if (is_ret_user_type) {
		/* hidden return value */
		llvm_args.push_back(code_builder->getTypePassStructPtr());
	}


	return true;
}

void Func::genCode(void) const
{
	EvalCtx*			ectx;
	llvm::Function			*llvm_f;
	llvm::BasicBlock		*f_bb;
	FuncArgs			*fargs;

	llvm_f = code_builder->getModule()->getFunction(getName());
	if (llvm_f == NULL) {
		cerr	<< "Could not find function prototype for "
			<< getName() << endl;
		return;
	}

	fargs = new FuncArgs(getArgs());

	/* scope takes args */
	ectx = new EvalCtx(fargs);
	
	f_bb = llvm::BasicBlock::Create(
		llvm::getGlobalContext(), "entry", llvm_f);

	code_builder->getBuilder()->SetInsertPoint(f_bb);

	/* set gen_func so that expr id resolution will get the right
	 * alloca variable */
	gen_func = this;

	/* create arguments */
	genLoadArgs();

	/* gen rest of code */
	block->codeGen(ectx);

	gen_func = NULL;
	gen_func_block = NULL;

	delete ectx;
	delete fargs;
}


