#include "thunk_fieldoffset_cond.h"
#include "thunk_type.h"
#include "code_builder.h"

extern CodeBuilder	*code_builder;

using namespace std;

ThunkFieldOffsetCond::~ThunkFieldOffsetCond(void)
{
	delete cond_expr;
	delete true_off;
	if (false_off != NULL) delete false_off;
}

const string ThunkFieldOffsetCond::getPresentFCallName(void) const
{
	return string("__ispresent_") + getOwner()->getType()->getName()
		+ "_" + getFieldName();
}

FCall* ThunkFieldOffsetCond::copyFCallPresent(void) const
{
	return new FCall(
		new Id(getPresentFCallName()),
		getOwner()->copyExprList());
}

ThunkFieldOffsetCond* ThunkFieldOffsetCond::copy(void) const
{
	ThunkFieldOffsetCond	*ret;

	ret = new ThunkFieldOffsetCond(
		cond_expr->copy(),
		true_off->copy(),
		(false_off) ? false_off->copy() : NULL);

	ret->setOwner(getOwner());
	ret->setFieldName(getFieldName());

	return ret;
}

bool ThunkFieldOffsetCond::genProto(void) const
{
	code_builder->genThunkProto(getFCallName());
	code_builder->genThunkProto(getPresentFCallName());
	return true;
}

bool ThunkFieldOffsetCond::genCode(void) const
{
	Expr	*false_expr = NULL;

	if (false_off == NULL)
		false_expr = new Number(0);
	code_builder->genCodeCond(
		getOwner()->getType(),
		getFCallName(),
		cond_expr,
		true_off,
		(false_off) ? false_off : false_expr);
	if (false_expr != NULL) {
		delete false_expr;
		false_expr = NULL;
	}

	/* generate present expr */
	if (false_off != NULL) {
		Expr	*not_present_expr, *present_expr;

		not_present_expr = new Number(0);
		present_expr = new Number(1);

		code_builder->genCodeCond(
			getOwner()->getType(),
			getPresentFCallName(),
			cond_expr,
			present_expr,
			not_present_expr);
			
		delete not_present_expr;
		delete present_expr;
	} else {
		Expr	*present_expr = new Number(1);
		code_builder->genCode(
			getOwner()->getType(),
			getPresentFCallName(),
			present_expr);
		delete present_expr;
	}
		

	return true;
}
