#include <assert.h>
#include <iostream>

#include <stdint.h>
#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/IRBuilder.h>

#include "eval.h"
#include "func.h"

using namespace std;

bool xxx_debug_eval = false;
extern ptype_map	ptypes_map;

static Expr* expr_resolve_ids(const EvalCtx& ectx, const Expr* expr);
static Expr* eval_rewrite_sizeof(const EvalCtx& ectx, const FCall* fc);

class ExprRewriteConsts : public ExprRewriteAll
{
public:
	ExprRewriteConsts(const const_map& consts)
	: constants(consts) {}

	virtual Expr* visit(const Id* id) 
	{
		Expr	*new_expr;
		new_expr = getNewExpr(id);
		if (new_expr == NULL)
			return id->copy();
		return new_expr;
	}

private:
	Expr* getNewExpr(const Id* id)
	{
		const_map::const_iterator	it;
		
		it = constants.find(id->getName());
		if (it == constants.end())
			return NULL;

		return ((*it).second)->copy();
	}
	
	const const_map& constants;

};

class ExprResolveIds : public ExprRewriteAll
{
public:
	ExprResolveIds(const EvalCtx& evalctx)
		: ectx(evalctx) {}
	
	virtual Expr* visit(const Id* id)
	{
		Expr	*result;
		result = ectx.resolve(id);
		if (result == NULL) 
			return id->copy();
		return result;
	}

	virtual Expr* visit(const IdStruct* ids)
	{	
		Expr	*result;

		result = ectx.resolve(ids);
		if (result == NULL)
			return ids->copy();

		return result;
	}

	virtual Expr* visit(const IdArray* ida)
	{
		Expr	*result;

		result = ectx.resolve(ida);
		if (result == NULL)
			return ida->copy();

		return result;
	}
	
	virtual Expr* visit(const FCall* fc)
	{
		Expr	*new_expr;
		
		if (fc->getName() == "sizeof") {
			Expr	*ret;
			ret = eval_rewrite_sizeof(ectx, fc);
			return ret;
		} 

		return ExprRewriteAll::visit(fc);
	}

private:
	const EvalCtx& ectx;
};

static Expr* expr_resolve_ids(const EvalCtx& ectx, const Expr* expr)
{
	ExprResolveIds	eri(ectx);
	return eri.apply(expr);
}


/* return non-null if new expr is allocated to replace cur_expr */
Expr* expr_resolve_consts(const const_map& consts, Expr* cur_expr)
{
	ExprRewriteConsts	erc(consts);
	return erc.apply(cur_expr);
}


static Expr* eval_rewrite_sizeof(const EvalCtx& ectx, const FCall* fc)
{
	const ExprList			*exprs;
	Expr				*front;
	Id				*front_id;
	PhysicalType			*pt;
	ptype_map::const_iterator	it;

	exprs = fc->getExprs();
	if (exprs->size() != 1) {
		cerr <<  "sizeof expects 1 argument. Got: ";
		fc->print(cerr);
		cerr << endl;
		return NULL;
	}

	front = exprs->front();
	front_id = dynamic_cast<Id*>(front);
	if (front_id == NULL) {
		cerr << "sizeof expects id for argument. Got: ";
		fc->print(cerr);
		cerr << endl;
		return NULL;
	}

	it = ptypes_map.find(front_id->getName());
	pt = (it == ptypes_map.end()) ? NULL : (*it).second;
	if (pt != NULL)
		return pt->getBytes();

	it = ptypes_map.find(string("thunk_")+front_id->getName()) ;
	pt = (it == ptypes_map.end()) ? NULL : (*it).second;
	if (pt == NULL) {
		cerr	<< "Could not find type for " << front_id->getName() 
			<< endl;
		return NULL;
	}

	/* XXX this is a thunk now, be smarter here */
	return pt->getBytes();
}

llvm::Value* evalAndGen(const EvalCtx& ectx, const Expr* expr)
{
	Expr		*ret;
	llvm::Value	*v;

	ret = eval(ectx, expr);
	if (ret == NULL)
		return NULL;

	v = ret->codeGen();
	delete ret;

	return v;
}

Expr* eval(const EvalCtx& ectx, const Expr* expr)
{
	Expr	*our_expr, *tmp_expr;
	
	/* first, get simplified copy */
	our_expr = expr->simplify();

	/* resolve all labeled constants into numbers */
	tmp_expr = expr_resolve_consts(ectx.getConstants(), our_expr);
	if (tmp_expr != NULL) {
		delete our_expr;
		our_expr = tmp_expr;
	}

	/* 
	 * resolve all unknown ids into function calls
	 */
	tmp_expr = expr_resolve_ids(ectx, our_expr);
	if (tmp_expr != NULL) {
		delete our_expr;
		our_expr = tmp_expr;
	}

	tmp_expr = expr_resolve_consts(ectx.getConstants(), our_expr);
	if (tmp_expr != NULL) {
		delete our_expr;
		our_expr = tmp_expr;
	}

	if (*our_expr != expr) {
		our_expr = evalReplace(ectx, our_expr);
	}

	return our_expr;
}

Expr* evalReplace(const EvalCtx& ectx, Expr* expr)
{
	Expr	*new_expr;

	if (expr == NULL) return NULL;

	new_expr = eval(ectx, expr);
	delete expr;
	return new_expr;
}

