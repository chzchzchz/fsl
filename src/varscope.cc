#include "type.h"
#include "varscope.h"
#include "symtab.h"
#include "runtime_interface.h"
#include "code_builder.h"
#include "util.h"
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


void VarScope::genTypeArgs(
	const Type* 		t,
	llvm::IRBuilder<>	*tmpB)
{
	unsigned int			arg_c;
	const llvm::Type		*l_t;
	const ArgsList			*args;
	llvm::AllocaInst		*params_ai;

	args = t->getArgs();
	if (args == NULL) return;
	arg_c = args->size();

	/* long type */
	l_t = llvm::Type::getInt64Ty(llvm::getGlobalContext());

	params_ai = vars_map[rt_glue.getThunkArgParamPtrName()].tv_ai;
	assert (params_ai != NULL && "Didn't allocate params?");

	for (unsigned int i = 0; i < arg_c; i++) {
		string				arg_name;
		string				arg_type;
		llvm::AllocaInst		*allocai;
		llvm::Value			*idx_val;
		llvm::Value			*param_elem_val;
		llvm::Value			*param_elem_ptr;

		arg_type = (args->get(i).first)->getName();
		arg_name = (args->get(i).second)->getName();

		if (symtabs.count(arg_type) != 0) {
			/* XXX TODO SUPPORT TYPE ARGS! */
			cerr << t->getName() << ": ";
			cerr << "Type args only supports scalar args" << endl;
			exit(-1);
		}

		allocai = tmpB->CreateAlloca(l_t, 0, arg_name);

		idx_val = llvm::ConstantInt::get(
			llvm::getGlobalContext(),
			llvm::APInt(32, i));

		param_elem_ptr = builder->CreateGEP(
			builder->CreateLoad(params_ai),
			idx_val);

		param_elem_val = builder->CreateLoad(param_elem_ptr);

		builder->CreateStore(param_elem_val, allocai);
		vars_map[arg_name] = TypedVar(NULL, allocai);
	}
}

void VarScope::loadClosureIntoThunkVars(
	llvm::Function::arg_iterator 	ai,
	llvm::IRBuilder<>		&tmpB)
{
	const llvm::Type		*l_t;
	llvm::AllocaInst		*allocai;

	/* create the hidden argument __thunk_off_arg */
	l_t = builder->getInt64Ty();
	allocai = tmpB.CreateAlloca(l_t, 0, rt_glue.getThunkArgOffsetName());
	vars_map[rt_glue.getThunkArgOffsetName()] = TypedVar(NULL, allocai);
	builder->CreateStore(
		builder->CreateExtractValue(
			builder->CreateLoad(ai), RT_CLO_IDX_OFFSET, "t_offset"),
		allocai);

	/* __thunk_params */
	l_t = llvm::Type::getInt64PtrTy(llvm::getGlobalContext());
	allocai = tmpB.CreateAlloca(l_t, 0, rt_glue.getThunkArgParamPtrName());
	vars_map[rt_glue.getThunkArgParamPtrName()] = TypedVar(NULL, allocai);
	builder->CreateStore(
		builder->CreateExtractValue(
			builder->CreateLoad(ai), RT_CLO_IDX_PARAMS, "t_params"),
		allocai);

	/* __thunk_virt */
	l_t = builder->getInt8PtrTy();
	allocai = tmpB.CreateAlloca(l_t, 0, rt_glue.getThunkArgVirtName());
	vars_map[rt_glue.getThunkArgVirtName()] = TypedVar(NULL, allocai);
	builder->CreateStore(
		builder->CreateExtractValue(
			builder->CreateLoad(ai), RT_CLO_IDX_XLATE, "t_virt"),
		allocai);

	/* __thunk_closure */
	allocai = tmpB.CreateAlloca(
		code_builder->getClosureTyPtr(), 0,
		rt_glue.getThunkClosureName());
	/* XXX! Should know the type of this.. */
	vars_map[rt_glue.getThunkClosureName()] = TypedVar(NULL, allocai);
	builder->CreateStore(ai, allocai);
}

void VarScope::genArgs(
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
	string			our_name, our_name_ptr;

	if (name == "") {
		our_name = "__clo_" + int_to_string(closure_c);
		our_name_ptr = "__ptr_" + our_name;
		closure_c++;
	} else {
		our_name = name;
		our_name_ptr = "__ptr_" + name;
	}

	ai = builder->CreateAlloca(
		code_builder->getClosureTy(),
		0, our_name);

	ai_ptr = builder->CreateAlloca(
		code_builder->getClosureTyPtr(),
		0, our_name_ptr);
	builder->CreateStore(ai, ai_ptr);
	vars_map[our_name_ptr] = TypedVar(t, ai_ptr);
	vars_map[our_name] = TypedVar(t, ai_ptr);
	builder->CreateStore(
		builder->CreateInsertValue(
			builder->CreateLoad(ai),
			code_builder->getNullPtrI8(),
			RT_CLO_IDX_XLATE),
		ai);

	/* no params, no need to allocate space on stack */
	if (t->getNumArgs() == 0)
		return ai_ptr;

	params_ai = code_builder->createPrivateTmpI64Array(
		t->getNumArgs(), our_name + "_params");
	builder->CreateStore(
		builder->CreateInsertValue(
			builder->CreateLoad(ai),
			params_ai,
			RT_CLO_IDX_PARAMS),
		ai);

	return ai_ptr;
}

void VarScope::print(void) const
{
	cerr << "DUmping tmpvars" << endl;
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
