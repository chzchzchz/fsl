#include <stdint.h>
#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/IRBuilder.h>
#include <iostream>

#include "type.h"
#include "code_builder.h"
#include "func.h"
#include "expr.h"
#include "util.h"
#include "runtime_interface.h"

using namespace std;
using namespace llvm;

extern CodeBuilder	*code_builder;
extern const Func	*gen_func;
extern const FuncBlock	*gen_func_block;
extern type_map		types_map;
extern typenum_map	typenums_map;
extern func_map		funcs_map;
extern RTInterface	rt_glue;


Expr* FCall::mkClosure(Expr* diskoff, Expr* params)
{
	assert (diskoff != NULL && params != NULL);
	return new FCall(new Id("__mkClosure"), new ExprList(diskoff, params));
}

llvm::Value* FCall::codeGenLet(void) const
{
	ExprList::const_iterator	it;
	Id				*to_bind_name;
	Expr				*to_bind_val;
	Expr				*to_apply;

	assert (id->getName() == "let");

	/* special form XXX */
	/* let(var_to_bind, value_to_bind_to_var, expr) */
	if (exprs->size() != 3) {
		return ErrorV(string("let takes 3 params"));
	}

	it = exprs->begin();
	to_bind_name = dynamic_cast<Id*>(*it);
	
	it++;
	to_bind_val = dynamic_cast<Expr*>(*it);

	it++;
	to_apply = dynamic_cast<Expr*>(*it);

	assert (to_bind_name != NULL);
	assert (to_bind_val != NULL);
	assert (to_apply != NULL);

	/* create new var in symbol table that contains the
	 * value of val */

	/* NOTE: Must know what sort of value we want to store */
	assert (0 == 1);

}

llvm::Value* FCall::codeGenExtractOff(void) const
{
	Expr		*expr;
	llvm::Value	*closure_val;
	llvm::Value	*ret;

	assert (id->getName() == "__extractOff");

	if (exprs->size() != 1) {
		cerr << "__extractOff takes 1 param" << endl;
		return NULL;
	}

	/* we were passed expr that is a TypePassStruct */
	expr = exprs->front();
	closure_val = expr->codeGen();
	if (closure_val == NULL) {
		cerr << "__extractOff could not get closure" << endl;
		return NULL;
	}

	ret = code_builder->getBuilder()->CreateExtractValue(
		code_builder->getBuilder()->CreateLoad(closure_val),
		0,
		"offset");

	return ret;
}

llvm::Value* FCall::codeGenExtractParam(void) const
{
	Expr		*expr;
	llvm::Value	*closure_val;

	assert (id->getName() == "__extractParam");

	if (exprs->size() != 1) {
		cerr << "__extractParam takes 1 param" << endl;
		return NULL;
	}

	/* we were passed expr that is a TypePassStruct */
	expr = exprs->front();
	closure_val = expr->codeGen();
	if (closure_val == NULL) {
		cerr << "__extractParam could not get closure";
		return NULL;
	}

	return code_builder->getBuilder()->CreateExtractValue(
		code_builder->getBuilder()->CreateLoad(closure_val),
		1,
		"params");
}

llvm::Value* FCall::codeGenDynClosure(void) const
{
	/* allocates a temporary closure structure that we can copy
	 * data into! */
	llvm::AllocaInst	*closure;
	Number			*n;
	FCall			*new_call;
	llvm::IRBuilder<>	*builder;
	unsigned long		type_num;

	assert (exprs->size() == 1);

	n = dynamic_cast<Number*>(exprs->front());
	assert (n != NULL);
	type_num = n->getValue();

	builder = code_builder->getBuilder();
	closure = code_builder->createTmpClosure(typenums_map[type_num]);

	new_call = new FCall(
		new Id("__getDynClosure"),
		new ExprList(n->copy(), new Id(closure->getName())));

	new_call->codeGen();
	delete new_call;

	if (new_call == NULL)
		return NULL;

	return builder->CreateLoad(closure);
}

/**
 * returns a pointer to array of params as the value
 */
llvm::Value* FCall::codeGenDynParams(void) const
{
	/* __getDynParams needs a parambuf to spill everything into 
	 * for return.
	 *
	 * generate an alloca beforehand based on the type we're calling
	 * into.. */
	llvm::AllocaInst	*parambuf;
	llvm::AllocaInst	*parambuf_ptr;
	Number			*n;
	FCall			*new_call;
	llvm::IRBuilder<>	*builder;
	unsigned long		type_num;

	assert (exprs->size() == 1);

	n = dynamic_cast<Number*>(exprs->front());
	assert (n != NULL);
	type_num = n->getValue();

	builder = code_builder->getBuilder();

	parambuf = code_builder->createPrivateTmpI64Array(
		typenums_map[type_num]->getNumArgs(),
		"dynparam");

	parambuf_ptr = code_builder->createTmpI64Ptr();
	builder->CreateStore(
		builder->CreateGEP(
			parambuf,
			llvm::ConstantInt::get(
				llvm::getGlobalContext(),
				llvm::APInt(32, 0))),
		parambuf_ptr);

	new_call = new FCall(
		new Id("__getDynParams"), 
		new ExprList(n->copy(), new Id(parambuf_ptr->getName())));
	new_call->codeGen();
	delete new_call;

	if (new_call == NULL)
		return NULL;

	return builder->CreateLoad(parambuf_ptr);
}

llvm::Value* FCall::codeGenMkClosure(void) const
{
	llvm::Value			*diskoff;
	llvm::Value			*params;
	llvm::Type			*ret_type;
	llvm::AllocaInst 		*closure;
	llvm::Value			*closure_loaded;
	llvm::IRBuilder<>		*builder;
	ExprList::const_iterator	it;

	/* this is compiler generated-- do not warn user! crash crash */
	assert (exprs->size() == 2);

	it = exprs->begin();
	diskoff = (*it)->codeGen(); it++;
	params = (*it)->codeGen();

	if (diskoff == NULL || params == NULL) {
		cerr << "Couldn't mkpass: ";
		print(cerr);
		cerr << endl;
		
		return NULL;
	}

	builder = code_builder->getBuilder();
	ret_type = code_builder->getClosureTy();
	closure = builder->CreateAlloca(ret_type, 0, "mkpasser");
	
	closure_loaded = builder->CreateLoad(closure);

	builder->CreateStore(
		builder->CreateInsertValue(
			builder->CreateInsertValue(
				builder->CreateInsertValue(
					closure_loaded, params, 1),
				diskoff, 0),
			code_builder->getNullPtrI8(), 2),
		closure);

	
	return closure;
}

llvm::Value* FCall::codeGenParamsAllocaByCount(void) const
{
	llvm::AllocaInst	*parambuf;
	Number			*n;

	assert (exprs->size() == 1);

	n = dynamic_cast<Number*>(exprs->front());
	assert (n != NULL);

	parambuf = code_builder->createPrivateTmpI64Array(
		n->getValue(), "paramallocacount");

	return parambuf;
}	

/**
 * generate paramters to pass into the function call
 */
llvm::Value* FCall::codeGenParams(vector<llvm::Value*>& args) const
{
	ExprList::const_iterator	it;
	llvm::Value			*params_ret;
	llvm::IRBuilder<>		*builder;


	builder = code_builder->getBuilder();
	params_ret = NULL;
	args.clear();

	for (it = exprs->begin(); it != exprs->end(); it++) {
		Expr	*cur_expr;
		FCall	*fc;

		cur_expr = *it;
		fc = dynamic_cast<FCall*>(cur_expr);
		if (fc != NULL && fc->getName() == "paramsAllocaByCount") {
			llvm::AllocaInst	*p_ptr;
			llvm::Value		*params;

			params = cur_expr->codeGen();
			p_ptr = code_builder->createTmpI64Ptr();

			builder->CreateStore(
				builder->CreateGEP(
					params,
					llvm::ConstantInt::get(
						llvm::getGlobalContext(),
						llvm::APInt(32, 0))),
				p_ptr);
			
			args.push_back(params);
			params_ret = params;
			
			continue;
		}

		args.push_back(cur_expr->codeGen());
	}

	return params_ret;
}

bool FCall::handleSpecialForms(llvm::Value* &ret) const
{
	string	call_name(id->getName());

	if (call_name == "let")
		ret = codeGenLet();
	else if (call_name == "__extractOff")
		ret = codeGenExtractOff();
	else if (call_name == "__extractParam")
		ret = codeGenExtractParam();
	else if (call_name == "__getDynParams_preAlloca")
		ret = codeGenDynParams();
	else if (call_name == "__mkClosure")
		ret = codeGenMkClosure();
	else if (call_name == "paramsAllocaByCount")
		ret = codeGenParamsAllocaByCount();
	else if (call_name =="__getDynClosure_preAlloca")
		ret = codeGenDynClosure();
	else
		return false;

	return true;
}

/**
 * We have a function call that is returning a closure.
 * BUT we convert the function to use a parameter for returns to avoid 
 * any scoping issues with the parambuf. 
 */
Value* FCall::codeGenClosureRetCall(std::vector<llvm::Value*>& args) const
{
	const Func		*callee_func;
	IRBuilder<>		*builder;
	Value			*ret;
	AllocaInst		*tmp_tp;
	AllocaInst		*tmp_params;
	unsigned int		param_c;

	builder = code_builder->getBuilder();
	callee_func = funcs_map[id->getName()];
	assert (callee_func != NULL);

	param_c = types_map[callee_func->getRet()]->getNumArgs();
	
	tmp_tp = builder->CreateAlloca(code_builder->getClosureTy());
	tmp_params = code_builder->createPrivateTmpI64Array(
		param_c, "BURRITOS"); 
	builder->CreateInsertValue(
		builder->CreateLoad(tmp_tp), tmp_params, 1);


	args.push_back(tmp_tp);
	ret = builder->CreateCall(
		code_builder->getModule()->getFunction(id->getName()), 
		args.begin(), args.end());

	return tmp_tp;
}

Value* FCall::codeGen() const
{
	Function			*callee;
	Func				*callee_func;
	vector<Value*>			args;
	string				call_name(id->getName());
	Value				*params_ret;
	Value				*ret;
	bool				is_ret_closure;
	unsigned int			expected_arg_c;
	IRBuilder<>			*builder;

	builder = code_builder->getBuilder();
	if (handleSpecialForms(ret) == true)
		return ret;

	callee = code_builder->getModule()->getFunction(call_name);
	if (callee == NULL) {
		return ErrorV(
			(string("could not find function ") + 
			id->getName()).c_str());
	}

	if (funcs_map.count(call_name) != 0) {
		callee_func = funcs_map[call_name];
		is_ret_closure = (types_map.count(callee_func->getRet()) > 0);
	} else
		is_ret_closure = false;

	expected_arg_c = callee->arg_size() - ((is_ret_closure) ? 1 : 0);
	if (expected_arg_c != exprs->size()) {
		return ErrorV(
			(string("wrong number of function arguments in ") + 
			id->getName() + 
			string(". Expected ") + 
			int_to_string(expected_arg_c) + 
			" got " + int_to_string(exprs->size())).c_str());
	}

	params_ret = codeGenParams(args);

	if (is_ret_closure) {
		ret = codeGenClosureRetCall(args);
		assert (params_ret == NULL);
	} else {
		ret = builder->CreateCall(callee, args.begin(), args.end());
	}

	if (params_ret != NULL)
		return params_ret;

	return ret;
}
