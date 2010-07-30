#include <iostream>

#include "thunk_type.h"
#include "thunk_fieldsize.h"
#include "code_builder.h"
#include "symtab.h"

using namespace std;

extern symtab_map	symtabs;
extern CodeBuilder*	code_builder;

const std::string ThunkFieldSize::getFCallName(void) const
{
	/* TODO: support field sizes that are type size aliases */

	return "__thunkfieldsize_"+
		owner->getType()->getName() + "_" +
		fieldname;
}


FCall* ThunkFieldSize::copyFCall(void) const
{
	return new FCall(
		new Id(getFCallName()),
		owner->copyExprList());
}

Expr* ThunkFieldSize::copyConstValue(void) const
{
	assert (raw_expr != NULL);
	assert (t == NULL);
	assert (dynamic_cast<const Number*>(raw_expr) != NULL);

	return raw_expr->copy();
}

ThunkFieldSize* ThunkFieldSize::copy(void) const
{
	ThunkFieldSize	*tfs;

	if (t != NULL)
		tfs = new ThunkFieldSize(owner, fieldname, t);
	else
		tfs = new ThunkFieldSize(
			owner, fieldname, raw_expr->copy());

	return tfs;
}

bool ThunkFieldSize::genCode(void) const
{
	if (t == NULL) {
		code_builder->genCode(
			owner->getType(), getFCallName(), raw_expr);
	} else {
		const ThunkType	*tt;
		Expr		*gen_expr;

		tt = symtabs[t->getName()]->getThunkType();
		gen_expr = tt->getSize()->copyFCall();

		/* alias for typesize function call */
		code_builder->genCode(
			owner->getType(),
			getFCallName(), 
			tt->getSize()->copyFCall());
	}

	return true;

}

