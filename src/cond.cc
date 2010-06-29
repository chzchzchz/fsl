#include "phys_type.h"
#include "expr.h"
#include "cond.h"

static PhysicalType* cond_cmpop_resolve(
	const CmpOp*		cmpop,
	const ptype_map&	tm,
	PhysicalType		*t,
	PhysicalType		*f);

static PhysicalType* cond_bop_resolve(
	const BinBoolOp*	bop,
	const ptype_map& 	tm,
	PhysicalType*		t,
	PhysicalType*		f);



static PhysicalType* cond_cmpop_resolve(
	const CmpOp*		cmpop,
	const ptype_map&	tm,
	PhysicalType		*t,
	PhysicalType		*f)
{
	Expr		*lhs, *rhs;

	assert (cmpop != NULL);

	lhs = (cmpop->getLHS())->simplify();
	rhs = (cmpop->getRHS())->simplify();

	switch (cmpop->getOp()) {
	case CmpOp::EQ: return new PhysTypeCondEQ(lhs, rhs, t, f);
	case CmpOp::NE: return new PhysTypeCondNE(lhs, rhs, t, f);
	case CmpOp::LE: return new PhysTypeCondLE(lhs, rhs, t, f);
	case CmpOp::LT: return new PhysTypeCondLT(lhs, rhs, t, f);
	case CmpOp::GT: return new PhysTypeCondGT(lhs, rhs, t, f);
	case CmpOp::GE: return new PhysTypeCondGE(lhs, rhs, t, f);
	default:
		assert (0 == 1);
	}

	return NULL;
}

static PhysicalType* cond_bop_resolve(
	const BinBoolOp*	bop,
	const ptype_map& 	tm,
	PhysicalType*		t,
	PhysicalType*		f)
{
	const CondExpr*	cond_lhs;
	const CondExpr*	cond_rhs;

	cond_lhs = bop->getLHS();
	cond_rhs = bop->getRHS();

	assert (bop != NULL);

	if ((dynamic_cast<const BOPAnd*>(bop)) != NULL) {
		/* if LHS evaluates to true, do RHS */

		return cond_resolve(
			cond_lhs,
			tm,
			cond_resolve(cond_rhs, tm, t, f), 
			f);
	} else {
		/* must be an OR */
		assert (dynamic_cast<const BOPOr*>(bop) != NULL);

		return cond_resolve(
			cond_lhs,
			tm,
			t,
			cond_resolve(cond_rhs, tm, t, f));
	}

	/* should not happen */
	assert (0 == 1);
	return NULL;
}

/**
 * convert a condition into a physical type.
 */
PhysicalType* cond_resolve(
	const CondExpr* cond, 
	const ptype_map& tm,
	PhysicalType* t, PhysicalType* f)
{
	const CmpOp	*cmpop;
	const BinBoolOp	*bop;
	const FuncCond	*fcond;

	cmpop = dynamic_cast<const CmpOp*>(cond);
	if (cmpop != NULL) {
		return cond_cmpop_resolve(cmpop, tm, t, f);
	}

	bop = dynamic_cast<const BinBoolOp*>(cond);
	if (bop != NULL) 
		return cond_bop_resolve(bop, tm, t, f);

	fcond = dynamic_cast<const FuncCond*>(cond);
	assert (fcond != NULL);

	return new PhysTypeCondEQ(
		(fcond->getFC())->simplify(),
		new Number(1),
		t,
		f);
}
