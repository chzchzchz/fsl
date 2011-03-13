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
#include "typeclosure.h"
#include "memotab.h"

using namespace std;
using namespace llvm;

extern MemoTab		memotab;
extern CodeBuilder	*code_builder;
extern const Func	*gen_func;
extern const FuncBlock	*gen_func_block;
extern type_map		types_map;
extern typenum_map	typenums_map;
extern func_map		funcs_map;
extern RTInterface	rt_glue;


Expr* FCall::mkBaseClosure(const ::Type* t)
{
	assert (t->getNumArgs() == 0 && "STUB >0 args");
	return mkClosure(
		new Number(0),
		new Id("__NULLPTR"),
		new Id("__NULLPTR8"));
}

Expr* FCall::mkClosure(Expr* diskoff, Expr* params, Expr* virt)
{
	ExprList	*el;
	Expr		*ret;

	assert (diskoff != NULL && params != NULL && virt != NULL);

	el = new ExprList();
	el->add(diskoff);
	el->add(params);
	el->add(virt);

	ret = new FCall(new Id("__mkClosure"), el);

	return ret;
}

Expr* FCall::extractCloOff(const Expr* e)
{
	const FCall	*fc = dynamic_cast<const FCall*>(e);
	if (fc != NULL && fc->getName() == string("__mkClosure"))
		return (fc->getExprs()->front())->simplify();
	return new FCall(new Id("__extractOff"), new ExprList(e->simplify()));
}

Expr* FCall::extractCloParam(const Expr* e)
{
	const FCall	*fc = dynamic_cast<const FCall*>(e);
	if (fc != NULL && fc->getName() == "__mkClosure")
		return (fc->getExprs()->getNth(1))->simplify();
	return new FCall(new Id("__extractParam"), new ExprList(e->simplify()));
}

Expr* FCall::extractCloVirt(const Expr* expr)
{
	const FCall	*fc = dynamic_cast<const FCall*>(expr);
	if (fc != NULL && fc->getName() == "__mkClosure")
		return (fc->getExprs()->getNth(2))->simplify();
	return new FCall(new Id("__extractVirt"), new ExprList(expr->simplify()));
}

Expr* FCall::extractParamVal(const Expr* expr, const Expr* off)
{
	return new FCall(
		new Id("__extractParamVal"),
		new ExprList(expr->simplify(), off->simplify()));
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
	if (exprs->size() != 3) return ErrorV(string("let takes 3 params"));

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

	return NULL;
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

	TypeClosure	tc(closure_val);
	ret = tc.getOffset();

	return ret;
}

llvm::Value* FCall::codeGenExtractVirt(void) const
{
	Expr		*expr;
	llvm::Value	*closure_val;

	assert (id->getName() == "__extractVirt");

	if (exprs->size() != 1) {
		cerr << "__extractVirt takes 1 param" << endl;
		return NULL;
	}

	/* we were passed expr that is a closure */
	expr = exprs->front();
	closure_val = expr->codeGen();
	if (closure_val == NULL) {
		cerr << "__extractVirt could not get closure" << endl;
		return NULL;
	}

	TypeClosure	tc(closure_val);
	return tc.getXlate();
}

llvm::Value* FCall::codeGenExtractParamVal(void) const
{
	Expr				*expr, *off;
	Number				*n;
	ExprList::const_iterator	it;
	llvm::Value			*pb_val, *ret, *idx_val;
	llvm::IRBuilder<>		*builder;

	assert (id->getName() == "__extractParamVal");

	if (exprs->size() != 2) {
		cerr << "__extractParamVal takes 2 params" << endl;
		return NULL;
	}

	/* we were passed expr that is a parambuf */
	it = exprs->begin();
	expr = (*it); it++;
	pb_val = expr->codeGen();
	off = (*it);

	if (pb_val == NULL) {
		cerr << "__extractParamVal could not get pbuf";
		return NULL;
	}

	n = dynamic_cast<Number*>(off);
	assert (n != NULL && "NEED NUMERIC PARAMVAL");

	builder = code_builder->getBuilder();
	idx_val = llvm::ConstantInt::get(
		llvm::getGlobalContext(),
		llvm::APInt(32, n->getValue()));
	ret = builder->CreateGEP(pb_val, idx_val);
	return builder->CreateLoad(ret);
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

	TypeClosure	tc(closure_val);
	return tc.getParamBuf();
}

llvm::Value* FCall::codeGenMkClosure(void) const
{
	llvm::Value			*diskoff, *params, *virt;
	llvm::Type			*ret_type;
	llvm::AllocaInst 		*closure;
	llvm::IRBuilder<>		*builder;
	ExprList::const_iterator	it;

	/* this is compiler generated-- do not warn user! crash crash */
	assert (exprs->size() == 3 && "WRONGLY SPECIFIED MKCLOSURE BY FSL");

	it = exprs->begin();
	diskoff = (*it)->codeGen(); it++;
	params = (*it)->codeGen(); it++;
	virt = (*it)->codeGen();

	if (diskoff == NULL || params == NULL) {
		cerr << "Couldn't mkpass: ";
		print(cerr);
		cerr << endl;
		return NULL;
	}

	builder = code_builder->getBuilder();
	ret_type = code_builder->getClosureTy();
	closure = builder->CreateAlloca(ret_type, 0, "mkpasser");

	TypeClosure	tc(closure);

	builder->CreateStore(
		tc.setAll(diskoff, params, virt),
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
		n->getValue(), "param_alloca_by_count");

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
	else if (call_name == "__extractParamVal")
		ret = codeGenExtractParamVal();
	else if (call_name == "__extractVirt")
		ret = codeGenExtractVirt();
	else if (call_name == "__mkClosure")
		ret = codeGenMkClosure();
	else if (call_name == "paramsAllocaByCount")
		ret = codeGenParamsAllocaByCount();
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

	param_c = types_map[callee_func->getRet()]->getParamBufEntryCount();

	tmp_tp = builder->CreateAlloca(code_builder->getClosureTy());
	tmp_params = code_builder->createPrivateTmpI64Array(
		param_c, "BURRITOS");
	TypeClosure	tc(tmp_tp);

	builder->CreateStore(tc.setParamBuf(tmp_params), tmp_tp);

	args.push_back(tmp_tp);
	ret = builder->CreateCall(
		code_builder->getModule()->getFunction(id->getName()),
		args.begin(), args.end());

	return tmp_tp;
}

llvm::Value* FCall::codeGenPrimitiveRetCall(vector<llvm::Value*>& args) const
{
	string		call_name(id->getName());
	Function	*callee;
	Func		*callee_func;
	IRBuilder<>	*builder;

	builder = code_builder->getBuilder();

	callee = code_builder->getModule()->getFunction(call_name);
	callee_func = NULL;
	if (funcs_map.count(call_name) != 0)
		callee_func = funcs_map[call_name];

	if (memotab.canMemoize(callee_func)) {
		/* idempotent function */
		return memotab.memoFuncCall(callee_func);
	}

	return builder->CreateCall(callee, args.begin(), args.end());
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
	} else {
		callee_func = NULL;
		is_ret_closure = false;
	}

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
		ret = codeGenPrimitiveRetCall(args);
	}

	if (params_ret != NULL) return params_ret;

	return ret;
}
