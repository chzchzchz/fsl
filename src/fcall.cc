#include <stdint.h>
#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/IRBuilder.h>
#include <iostream>

#include "code_builder.h"
#include "func.h"
#include "expr.h"
#include "util.h"

using namespace std;
using namespace llvm;

extern CodeBuilder	*code_builder;
extern const Func	*gen_func;
extern const FuncBlock	*gen_func_block;


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
	llvm::Value	*typepass_val;

	assert (id->getName() == "__extractOff");

	if (exprs->size() != 1) {
		cerr << "__extractOff takes 1 param" << endl;
		return NULL;
	}

	/* we were passed expr that is a TypePassStruct */
	expr = exprs->front();
	typepass_val = expr->codeGen();
	if (typepass_val == NULL) {
		cerr << "__extractOff could not get typepass" << endl;
		return NULL;
	}

	return code_builder->getBuilder()->CreateExtractValue(
		typepass_val, 0, "offset");
}

llvm::Value* FCall::codeGenExtractParam(void) const
{
	Expr		*expr;
	llvm::Value	*typepass_val;

	assert (id->getName() == "__extractParam");

	if (exprs->size() != 1) {
		cerr << "__extractParam takes 1 param" << endl;
		return NULL;
	}

	/* we were passed expr that is a TypePassStruct */
	expr = exprs->front();
	typepass_val = expr->codeGen();
	if (typepass_val == NULL) {
		cerr << "__extractParam could not get typepass";
		return NULL;
	}

	return code_builder->getBuilder()->CreateExtractValue(
		typepass_val, 1, "params");
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
	::Type			*t;
	Expr			*param_ptr;
	FCall			*new_call;
	ExprList		*new_exprs;
	llvm::IRBuilder<>	*builder;

	assert (exprs->size() == 1);

	n = dynamic_cast<Number*>(exprs->front());
	assert (n != NULL);

	builder = code_builder->getBuilder();

	parambuf = code_builder->createPrivateTmpI64Array(
		n->getValue(), "dynparam");

	parambuf_ptr = code_builder->createTmpI64Ptr();
	builder->CreateStore(
		builder->CreateGEP(
			parambuf,
			llvm::ConstantInt::get(
				llvm::getGlobalContext(),
				llvm::APInt(32, 0))),
		parambuf_ptr);

	new_exprs = new ExprList();
	new_exprs->add(n->copy());
	new_exprs->add(new Id(parambuf_ptr->getName()));

	new_call = new FCall(new Id("__getDynParams"), new_exprs);
	new_call->codeGen();
	delete new_call;

	if (new_call == NULL)
		return NULL;

	return builder->CreateLoad(parambuf_ptr);
}

llvm::Value* FCall::codeGenMkTypePass(void) const
{
	llvm::Value			*diskoff;
	llvm::Value			*params;
	llvm::Type			*ret_type;
	llvm::AllocaInst 		*typepass;
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
	ret_type = code_builder->getTypePassStruct();
	typepass = builder->CreateAlloca(ret_type, 0, "mkpasser");

	builder->CreateInsertValue(typepass, diskoff, 0);
	builder->CreateInsertValue(typepass, params, 1);
	
	return builder->CreateLoad(typepass);
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

llvm::Value* FCall::codeGenParams(vector<llvm::Value*>& args) const
{
	ExprList::const_iterator	it;
	llvm::Value			*params_ret;
	llvm::IRBuilder<>		*builder;


	builder = code_builder->getBuilder();
	params_ret = NULL;
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
			
//			args.push_back(p_ptr);
			p_ptr->dump();
//			params_ret = builder->CreateLoad(p_ptr);
			args.push_back(params);
			params_ret = params;
			
			continue;
		}

		args.push_back(cur_expr->codeGen());
	}

	return params_ret;
}

Value* FCall::codeGen() const
{
	Function			*callee;
	vector<Value*>			args;
	string				call_name(id->getName());
	Value				*params_ret;
	Value				*ret;
	Value				*parent;

	cerr << "ENTERING CODEGEN FOR " << endl;
	print(cerr);
	cerr << endl;

	if (call_name == "let") {
		return codeGenLet();
	} else if (call_name == "__extractOff") {
		return codeGenExtractOff();
	} else if (call_name == "__extractParam") {
		return codeGenExtractParam();
	} else if (call_name == "__getDynParams_preAlloca") {
		return codeGenDynParams();
	}else if (call_name == "__mktypepass") {
		return codeGenMkTypePass();
	} else if (call_name == "paramsAllocaByCount") {
		return codeGenParamsAllocaByCount();
	}

	callee = code_builder->getModule()->getFunction(call_name);
	if (callee == NULL) {
		return ErrorV(
			(string("could not find function ") + 
			id->getName()).c_str());
	}

	if (callee->arg_size() != exprs->size()) {
		return ErrorV(
			(string("wrong number of function arguments in ") + 
			id->getName() + 
			string(". Expected ") + 
			int_to_string(callee->size()) + 
			" got " + int_to_string(exprs->size())).c_str());
	}

	BasicBlock	*bb;
	bb = code_builder->getBuilder()->GetInsertBlock();
	assert (bb != NULL);
	parent = bb->getParent();
	if (parent != NULL) 
		cerr << "-----------PARENT: " << (string)parent->getName() << endl;
	else
		cerr << "----------PARENT: " << "???" << endl;

	params_ret = codeGenParams(args);

	cerr << "CALLING: ";
	callee->dump();

	cerr << "Dumping args: " <<  endl;
	for (	vector<Value*>::iterator it = args.begin();
		it != args.end();
		it++)
	{
		cerr << "Arg: ";
		(*it)->dump();
		cerr << endl;
	}
	cerr << "End Args" << endl;

	ret = code_builder->getBuilder()->CreateCall(
		callee, args.begin(), args.end());
	cerr << "Call created." << endl;

	if (params_ret != NULL)
		return params_ret;

	return ret;
}


