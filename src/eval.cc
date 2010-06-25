#include <assert.h>
#include <iostream>

#include <stdint.h>
#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/IRBuilder.h>

#include "eval.h"

using namespace std;
using namespace llvm;

static Expr* expr_resolve_ids(const EvalCtx& ectx, Expr* expr);

/* return non-null if new expr is allocated to replace cur_expr */
Expr* expr_resolve_consts(const const_map& consts, Expr* cur_expr)
{
	ExprParens*	ep;
	Id*		id;
	BinArithOp*	bop;
	FCall*		fc;

	ep = dynamic_cast<ExprParens*>(cur_expr);
	id = dynamic_cast<Id*>(cur_expr);
	bop = dynamic_cast<BinArithOp*>(cur_expr);
	fc = dynamic_cast<FCall*>(cur_expr);

	if (ep != NULL) {
		Expr	*inner_expr;
		Expr	*ret;

		inner_expr = ep->getExpr();
		ret = expr_resolve_consts(consts, inner_expr);
		if (ret != NULL) {
			ep->setExpr(ret);
		}

		return NULL;
	} else if (id != NULL) {
		Expr*	const_expr;
		
		const_expr = (*(consts.find(id->getName()))).second;
		if (const_expr == NULL) {
			return NULL;
		}

		return const_expr->copy();
	} else if (bop != NULL) {
		Expr	*ret;

		ret = expr_resolve_consts(consts, bop->getLHS());
		if (ret != NULL) {
			bop->setLHS(ret);
		}

		ret = expr_resolve_consts(consts, bop->getRHS());
		if (ret != NULL) {
			bop->setRHS(ret);
		}

		return bop->simplify();
	} else if (fc != NULL) {
		Expr*		ret;
		ExprList	*new_el;
		const ExprList	*old_el;

		old_el = fc->getExprs();
		new_el = new ExprList();
		for (	ExprList::const_iterator it = old_el->begin(); 
			it != old_el->end();
			it++)
		{
			Expr*	new_expr;
			
			new_expr = expr_resolve_consts(consts, *it);
			if (new_expr == NULL) {
				new_el->add((*it)->copy());
			} else {
				new_el->add(new_expr);
			}
		}

		/* TODO add support here to that we can change from function
		 * call to static value? */

		fc->setExprs(new_el);

		return NULL;
	} 

	/* don't know what to do with it, so don't touch it */
	return NULL;
}


/**
 * convert ids into functions that go back to the run-time
 */
static Expr* expr_resolve_ids(const EvalCtx& ectx, Expr* expr)
{
	Id		*id;
	IdStruct	*ids;
	IdArray		*ida;
	BinArithOp	*bop;
	FCall		*fc;

	id = dynamic_cast<Id*>(expr);
	ids = dynamic_cast<IdStruct*>(expr);
	ida = dynamic_cast<IdArray*>(expr);
	bop = dynamic_cast<BinArithOp*>(expr);
	fc = dynamic_cast<FCall*>(expr);

	if (id != NULL) {
		return ectx.resolve(id);
	} else if (ids != NULL) {
		return ectx.resolve(ids);
	} else if (ida != NULL) {
		return ectx.resolve(ida);
	} else if (bop != NULL) {
		Expr	*lhs, *rhs;
		Expr	*new_lhs, *new_rhs;

		lhs = bop->getLHS();
		rhs = bop->getRHS();

		new_lhs = expr_resolve_ids(ectx, lhs);
		new_rhs = expr_resolve_ids(ectx, rhs);

		if (new_lhs != NULL) bop->setLHS(new_lhs);
		if (new_rhs != NULL) bop->setRHS(new_rhs);

		return NULL;
	} else if (fc != NULL) {
		Expr*		ret;
		ExprList	*new_el;
		const ExprList	*old_el;

		old_el = fc->getExprs();
		new_el = new ExprList();
		for (	ExprList::const_iterator it = old_el->begin(); 
			it != old_el->end();
			it++)
		{
			Expr*	new_expr;
			
			new_expr = expr_resolve_ids(ectx, *it);
			if (new_expr == NULL) {
				new_el->add((*it)->copy());
			} else {
				new_el->add(new_expr);
			}
		}

		fc->setExprs(new_el);

		return NULL;
	}

	return NULL;
}

static Expr* expr_resolve_macros(const EvalCtx& ectx, Expr* expr)
{
	BinArithOp	*bop;
	FCall		*fc;

	bop = dynamic_cast<BinArithOp*>(expr);
	if (bop != NULL) {
		/* recurse */

		return NULL;
	}

	fc = dynamic_cast<FCall*>(expr);
	if (fc != NULL) {
		/* if it's a macro, apply the appropriate function */
	}

	
}

void eval(
	const EvalCtx& ectx,
	const Expr* expr)
{
	Expr	*our_expr, *tmp_expr;
	Expr	*base;
	
	/* first, get simplified copy */
	our_expr = expr->simplify();

	/* resolve all labeled constants into numbers */
	tmp_expr = expr_resolve_consts(ectx.getConstants(), our_expr);
	if (tmp_expr != NULL) {
		delete our_expr;
		our_expr = tmp_expr;
	}

	/* expand macros (e.g. sizeof, from_base, ...) */
	tmp_expr = expr_resolve_macros(ectx, our_expr);
	if (tmp_expr != NULL) {
		delete our_expr;
		our_expr = tmp_expr;
	}

	/* 
	 * XXX
	 * resolve all unknown ids into function calls. 
	 * ***THIS NEEDS SCOPING INFORMATION!!!***
	 * (e.g. id's that are not in the current scope)
	 */
	tmp_expr = expr_resolve_ids(ectx, our_expr);
	if (tmp_expr != NULL) {
		delete our_expr;
		our_expr = tmp_expr;
	}

	/**
	 * XXX
	 * resolve all 'from_base' function calls
	 */

	/**
	 * finally, codegen.
	 */

	/* next, codegen...  */
#if 0
	base = new Number(0);
	eval_recur(base, our_expr);
	delete base;
#endif
	assert (0 == 1);
}
