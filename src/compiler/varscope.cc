#include "type.h"
#include "varscope.h"
#include "symtab.h"
#include "runtime_interface.h"
#include "code_builder.h"
#include "util.h"
#include "typeclosure.h"
#include <iostream>


extern RTInterface	rt_glue;
extern symtab_map	symtabs;
extern CodeBuilder*	code_builder;
extern type_map		types_map;
using namespace std;


VarScope::VarScope(void)
: builder(code_builder->getBuilder()), tmp_c(0) {}

VarScope::VarScope(llvm::IRBuilder<>* in_b)
: builder(in_b), tmp_c(0) {}


/* pull args from parambuf */
void VarScope::genTypeArgs(const Type* t)
{
	const llvm::Type		*l_t;
	const ArgsList			*args;
	llvm::AllocaInst		*params_ai;

	args = t->getArgs();
	if (args == NULL) return;

	/* long type */
	l_t = llvm::Type::getInt64Ty(llvm::getGlobalContext());

	params_ai = vars_map[rt_glue.getThunkArgParamPtrName()].tv_ai;
	assert (params_ai != NULL && "Didn't allocate params?");

	loadArgsFromArray(params_ai, args);
}

void VarScope::loadClosureIntoThunkVars(
	llvm::Function::arg_iterator 	ai,
	llvm::IRBuilder<>		&tmpB)
{
	const llvm::Type		*l_t;
	llvm::AllocaInst		*allocai;
	TypeClosure			tc(ai);

	/* create the hidden argument __thunk_off_arg */
	l_t = builder->getInt64Ty();
	allocai = tmpB.CreateAlloca(l_t, 0, rt_glue.getThunkArgOffsetName());
	vars_map[rt_glue.getThunkArgOffsetName()] = TypedVar(NULL, allocai);
	builder->CreateStore(tc.getOffset(), allocai);

	/* __thunk_params */
	l_t = llvm::Type::getInt64PtrTy(llvm::getGlobalContext());
	allocai = tmpB.CreateAlloca(l_t, 0, rt_glue.getThunkArgParamPtrName());
	vars_map[rt_glue.getThunkArgParamPtrName()] = TypedVar(NULL, allocai);
	builder->CreateStore(tc.getParamBuf(), allocai);

	/* __thunk_virt */
	l_t = builder->getInt8PtrTy();
	allocai = tmpB.CreateAlloca(l_t, 0, rt_glue.getThunkArgVirtName());
	vars_map[rt_glue.getThunkArgVirtName()] = TypedVar(NULL, allocai);
	builder->CreateStore(tc.getXlate(), allocai);

	/* __thunk_closure */
	allocai = tmpB.CreateAlloca(
		code_builder->getClosureTyPtr(), 0,
		rt_glue.getThunkClosureName());
	/* XXX! Should know the type of this.. */
	vars_map[rt_glue.getThunkClosureName()] = TypedVar(NULL, allocai);
	builder->CreateStore(ai, allocai);
}

void VarScope::loadArgs(
	llvm::Function::arg_iterator	&ai,
	llvm::IRBuilder<>		*tmpB,
	const ArgsList			*args)
{
	unsigned int	arg_c;

	if (args == NULL) return;
	arg_c = args->size();

	for (unsigned int i = 0; i < arg_c; i++, ai++) {
		llvm::AllocaInst	*allocai;
		const llvm::Type	*t;
		const Type*		user_type;
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

		allocai = tmpB->CreateAlloca(t, 0, arg_name);
		code_builder->getBuilder()->CreateStore(ai, allocai);
		vars_map[arg_name] = TypedVar(user_type, allocai);
	}
}

void VarScope::loadArgsFromArray(
	llvm::Value			*ai,	/* just an array ptr */
	const ArgsList			*args)
{
	unsigned int	arg_c;
	unsigned int	array_idx;

	arg_c = args->size();
	array_idx = 0;

	for (unsigned int i = 0; i < arg_c; i++) {
		bool				is_user_type;
		const Type			*user_type;
		string				arg_name;
		string				arg_type;
		llvm::AllocaInst		*allocai;
		llvm::Value			*param_elem_val;

		arg_type = (args->get(i).first)->getName();
		arg_name = (args->get(i).second)->getName();

		is_user_type = (symtabs.count(arg_type) > 0);
		if (is_user_type) {
			user_type = types_map[arg_type];
			allocai = createTmpClosure(
				user_type, arg_name, ai, array_idx);
			array_idx += user_type->getParamBufEntryCount()+2;
		} else {
			param_elem_val = builder->CreateLoad(
				code_builder->loadPtr(ai, array_idx));
			allocai = createTmpI64(arg_name);
			builder->CreateStore(param_elem_val, allocai);
			array_idx++;
		}
	}
}

llvm::AllocaInst* VarScope::createTmpI64Ptr(void)
{
	llvm::AllocaInst	*ret;

	ret = builder->CreateAlloca(
		llvm::Type::getInt64PtrTy(llvm::getGlobalContext()),
		0,
		"__AAA" + int_to_string(tmp_c++));
	vars_map[ret->getName()] = TypedVar(NULL, ret);

	return ret;
}

llvm::AllocaInst* VarScope::getTmpAllocaInst(const std::string& s) const
{
	var_map::const_iterator	it;

	it = vars_map.find(s);
	if (it == vars_map.end())
		return NULL;

	return ((*it).second).tv_ai;
}

llvm::AllocaInst* VarScope::createTmpI64(const std::string& name)
{
	llvm::AllocaInst	*ret;

	ret = builder->CreateAlloca(builder->getInt64Ty(), 0, name);
	assert (name == ret->getName());
	vars_map[name] = TypedVar(NULL, ret);

	return ret;
}

llvm::AllocaInst* VarScope::createTmpClosurePtr(
	const Type* t, const std::string& name,
	llvm::Function::arg_iterator ai,
	llvm::IRBuilder<>	&tmpB)
{
	llvm::AllocaInst	*allocai;
	const llvm::Type	*tp;

	tp = code_builder->getClosureTyPtr();

	allocai = tmpB.CreateAlloca(tp, 0, name);
	vars_map[name] = TypedVar(t, allocai);
	builder->CreateStore(ai, allocai);

	return allocai;
}

static unsigned int closure_c = 0;
llvm::AllocaInst* VarScope::createTmpClosure(
	const Type* t, const std::string& name)
{
	llvm::AllocaInst	*ai, *params_ai, *ai_ptr;
	llvm::Value		*offset;
	string			our_name, our_name_ptr;

	if (name == "") {
		our_name = "__clo_" + int_to_string(closure_c);
		our_name_ptr = "__ptr_" + our_name;
		closure_c++;
	} else {
		our_name = name;
		our_name_ptr = "__ptr_" + name;
	}

	ai = builder->CreateAlloca(code_builder->getClosureTy(), 0, our_name);
	TypeClosure	tc(ai);

	ai_ptr = builder->CreateAlloca(
		code_builder->getClosureTyPtr(),
		0, our_name_ptr);
	vars_map[our_name_ptr] = TypedVar(t, ai_ptr);
	vars_map[our_name] = TypedVar(t, ai_ptr);

	/* no params, no need to allocate space on stack */
	if (t->getNumArgs() != 0) {
		/* allocate space for parambuf */
		params_ai = code_builder->createPrivateTmpI64Array(
			t->getParamBufEntryCount(), our_name + "_params");
	} else
		params_ai = NULL;

	offset = llvm::ConstantInt::get(
		llvm::getGlobalContext(),
		llvm::APInt(64, 0));

	builder->CreateStore(
		tc.setAll(offset, params_ai, NULL),
		ai);
	builder->CreateStore(ai, ai_ptr);

	return ai_ptr;
}

/* load a closure out of a parambuf */
llvm::AllocaInst* VarScope::createTmpClosure(
	const Type* t, const std::string& name,
	llvm::Value* parambuf_ai, unsigned int pb_base_idx)
{
	llvm::AllocaInst	*clo_ptr;
	llvm::LoadInst		*clo;
	llvm::Value		*clo_pb;
	llvm::Value		*off_val, *xlate_val;
	llvm::Value		*idx_val;

	clo_ptr = createTmpClosure(t, name);
	clo = builder->CreateLoad(clo_ptr);
	TypeClosure		tc(clo);

	/* offset */
	off_val = builder->CreateLoad(
		code_builder->loadPtr(parambuf_ai, pb_base_idx));
	builder->CreateStore(tc.setOffset(off_val), clo);

	/* xlate ptr */
	xlate_val = builder->CreateLoad(
		code_builder->loadPtr(parambuf_ai, pb_base_idx+1));
	builder->CreateStore(
		tc.setXlate(
			builder->CreateIntToPtr(
				xlate_val,
				builder->getInt8PtrTy())),
		clo);

	if (t->getNumArgs() == 0) return clo_ptr;

	/* dump out rest of values */
	clo_pb = tc.getParamBuf();
	idx_val = llvm::ConstantInt::get(
		llvm::getGlobalContext(),
		llvm::APInt(32, pb_base_idx+2));
	code_builder->emitMemcpy64(
		clo_pb,
		builder->CreateGEP(builder->CreateLoad(parambuf_ai), idx_val),
		t->getParamBufEntryCount());

	return clo_ptr;
}

void VarScope::print(void) const
{
	cerr << "Dumping varscope" << endl;
	for (	var_map::const_iterator it = vars_map.begin();
		it != vars_map.end();
		it++)
	{
		cerr << (*it).first << endl;
	}
	cerr << "Done dumping." << endl;
}

llvm::AllocaInst* VarScope::getVar(const std::string& s) const
{
	var_map::const_iterator	it;
	it = vars_map.find(s);
	if (it == vars_map.end()) return NULL;
	return ((*it).second).tv_ai;
}

const Type* VarScope::getVarType(const std::string& s) const
{
	var_map::const_iterator	it;
	it = vars_map.find(s);
	if (it == vars_map.end()) return NULL;
	return ((*it).second).tv_t;
}
