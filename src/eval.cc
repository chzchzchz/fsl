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

static Expr* expr_resolve_ids(const EvalCtx& ectx, Expr* expr);

/* return non-null if new expr is allocated to replace cur_expr */
Expr* expr_resolve_consts(const const_map& consts, Expr* cur_expr)
{
	ExprParens*	ep;
	Id*		id;
	IdArray*	ida;
	BinArithOp*	bop;
	FCall*		fc;

	ep = dynamic_cast<ExprParens*>(cur_expr);
	id = dynamic_cast<Id*>(cur_expr);
	ida = dynamic_cast<IdArray*>(cur_expr);
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
	} else if (ida != NULL) {
		Expr*	idx_expr;
		
		idx_expr = expr_resolve_consts(consts, ida->getIdx());
		if (idx_expr != NULL) {
			ida->setIdx(idx_expr);
		}

		return NULL;
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
			
			new_expr = eval(ectx, *it);
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

	return our_expr;
}

Expr* EvalCtx::resolve(const Id* id) const
{
	const_map::const_iterator	const_it;
	sym_binding			symbind;

	assert (id != NULL);

	/* is it a constant? */
	const_it = constants.find(id->getName());
	if (const_it != constants.end()) {
		return ((*const_it).second)->simplify();
	}
	
	
	/* is it in the current scope? */
	if (cur_scope.lookup(id->getName(), symbind) == true) {
		ExprList	*elist;

		elist = new ExprList();
		elist->add(symbind_off(symbind)->simplify());
		elist->add(symbind_phys(symbind)->getBits());

		return new FCall(new Id("__getLocal"), elist);
	}
	

	/* could not resolve */
	return NULL;
}

Expr* EvalCtx::getStructExpr(
	const SymbolTable		*first_symtab,
	const IdStruct::const_iterator	ids_first,
	const IdStruct::const_iterator	ids_end,
	const PhysicalType*		&final_type) const
{
	IdStruct::const_iterator	it;
	Expr				*ret;
	const SymbolTable		*cur_symtab;


	ret = new Number(0);
	cur_symtab = first_symtab;

	for (it = ids_first; it != ids_end; it++) {
		const Expr		*cur_expr = *it;
		std::string		cur_name;
		Expr			*cur_idx;
		PhysicalType		*cur_pt;
		PhysTypeArray		*pta;
		sym_binding		sb;
		const PhysTypeUser	*cur_ptu;

		if (cur_symtab == NULL) {
			/* no way we could possibly resolve the current 
			 * element. */
			delete ret;
			return NULL;
		}

		if (toName(cur_expr, cur_name, cur_idx) == false) {
			delete ret;
			return NULL;
		}

		if (cur_symtab->lookup(cur_name, sb) == false) {
			if (cur_idx != NULL) delete cur_idx;
			delete ret;
			return NULL;

		}

		cur_pt = symbind_phys(sb);
		pta = dynamic_cast<PhysTypeArray*>(cur_pt);
		if (pta != NULL) {
			/* it's an array */
			if (cur_idx == NULL) {
				cerr << "Array type but no index?" << endl;
				delete ret;
				return NULL;
			}

			ret = new AOPAdd(ret, pta->getBits(cur_idx));
			delete cur_idx;

			cur_ptu = dynamic_cast<const PhysTypeUser*>(
				pta->getBase());
		} else {
			/* it's a scalar */
			if (cur_idx != NULL) {
				cerr << "Tried to index a scalar?" << endl;
				delete cur_idx;
				delete ret;
				return NULL;
			}
			ret = new AOPAdd(ret, cur_pt->getBits());
			cur_ptu = dynamic_cast<const PhysTypeUser*>(cur_pt);
		}

		if (cur_ptu == NULL) {
			cur_symtab = NULL;
		} else {
			cur_symtab = symtabByName(
				(cur_ptu->getType())->getName());
		}

		final_type = cur_pt;
	}

	return ret;
}

Expr* EvalCtx::resolve(const IdStruct* ids) const
{	
	Expr				*offset;
	const PhysicalType		*ids_pt;
	const Id			*front_id;
	IdStruct::const_iterator	it;

	assert (ids != NULL);

	it = ids->begin();
	assert (it != ids->end());

	/* in current scope? */
	offset = getStructExpr(&cur_scope, ids->begin(), ids->end(), ids_pt);
	if (offset != NULL) {
		ExprList		*exprs;
		const PhysTypeArray	*pta;

		pta = dynamic_cast<const PhysTypeArray*>(ids_pt);

		exprs = new ExprList();
		exprs->add(offset->simplify());

		if (pta != NULL)
			exprs->add((pta->getBase())->getBits());
		else
			exprs->add(ids_pt->getBits());

		delete offset;

		return new FCall(new Id("__getLocal"), exprs);
	}

	/* global call? */
	it = ids->begin();
	front_id = dynamic_cast<const Id*>(*it);
	if (front_id != NULL) {
		const SymbolTable	*top_symtab;
		const Type		*top_type;

		top_type = typeByName(front_id->getName());
		top_symtab = symtabByName(front_id->getName());
		if (top_symtab == NULL || top_type == NULL)
			goto err;

		it++;
		offset = getStructExpr(top_symtab, it, ids->end(), ids_pt);
		if (offset != NULL) {
			ExprList		*exprs;
			const PhysTypeArray	*pta;

			pta = dynamic_cast<const PhysTypeArray*>(ids_pt);

			exprs = new ExprList();
			exprs->add(new Number(top_type->getTypeNum()));
			exprs->add(offset->simplify());

			if (pta != NULL)
				exprs->add((pta->getBase())->getBits());
			else
				exprs->add(ids_pt->getBits());

			delete offset;
			return new FCall(new Id("__getDyn"), exprs);
		}
	}

err:
	/* can't resolve it.. */
	cerr << "Can't resolve idstruct: ";
	ids->print(cerr);
	cerr << endl;

	return NULL;
}

Expr* EvalCtx::resolve(const IdArray* ida) const
{
	const_map::const_iterator	const_it;
	sym_binding			symbind;
	const PhysicalType		*pt;
	const PhysTypeArray		*pta;

	assert (ida != NULL);

	if (cur_scope.lookup(ida->getName(), symbind) == true) {
		/* array is in current scope */
		ExprList	*elist;
		Expr		*evaled_idx;

		cout << "Evaluating array index: ";
		ida->getIdx()->print(cout);
		cout << endl;

		evaled_idx = eval(*this, ida->getIdx());

		if (evaled_idx == NULL) {
			cerr << "Could not resolve ";
			(ida->getIdx())->print(cerr);
			cerr << endl;
			return NULL;
		}

		pt = symbind_phys(symbind);
		assert (pt != NULL);

		pta = dynamic_cast<const PhysTypeArray*>(pt);
		assert (pta != NULL);


		/* convert into __getLocalArray call */
		elist = new ExprList();
		elist->add(evaled_idx->simplify());
		elist->add((pta->getBase())->getBits());
		elist->add(symbind_off(symbind)->simplify());
		elist->add(symbind_phys(symbind)->getBits());

		delete evaled_idx;

		return new FCall(new Id("__getLocalArray"), elist);
	}
	
/* TODO -- add support for constant arrays */
	assert (0 == 1);
#if 0
	const_it = constants.find(id->getName());
	if (const_it != constants.end()) {
		return (const_it.second)->simplify();
	}
#endif
	
	
	/* could not resolve */
	return NULL;
}

bool EvalCtx::toName(const Expr* e, std::string& in_str, Expr* &idx) const
{
	const Id		*id;
	const IdArray		*ida;
	
	idx = NULL;
	id = dynamic_cast<const Id*>(e);
	if (id != NULL) {
		in_str = id->getName();
		return true;
	}

	ida = dynamic_cast<const IdArray*>(e);
	if (ida != NULL) {
		in_str = ida->getName();
		idx = (ida->getIdx())->simplify();
		return true;
	}

	return false;
}

const SymbolTable* EvalCtx::symtabByName(const std::string& s) const
{
	symtab_map::const_iterator	it;

	it = all_types.find(s);
	if (it == all_types.end())
		return NULL;

	return (*it).second;
}

const Type* EvalCtx::typeByName(const std::string& s) const
{
	const SymbolTable	*st;
	const Type		*t;
	const PhysicalType	*pt;
	const PhysTypeUser	*ptu;

	st = symtabByName(s);
	if (st == NULL)
		return NULL;

	pt = st->getOwner();
	assert (pt != NULL);

	ptu = dynamic_cast<const PhysTypeUser*>(pt);
	if (ptu == NULL)
		return NULL;

	return ptu->getType();
}
