#include "thunk_elems.h"
#include "thunk_type.h"

using namespace std;

bool ThunkElements::isSingleton(void) const
{
	const Number	*n;

	if ((n = dynamic_cast<const Number*>(raw_expr)) == NULL)
		return false;
	return (n->getValue() == 1);
}

const string ThunkElements::getFCallName(void) const
{
	return "__thunkelems_" +
		owner->getType()->getName() + "_" +
		fieldname;
}

FCall* ThunkElements::copyFCall(void) const
{
	return new FCall(
		new Id(getFCallName()),
		owner->copyExprList());
}

ThunkElements* ThunkElements::copy(void) const
{
	assert (raw_expr != NULL);
	return new ThunkElements(
		owner, fieldname, raw_expr->copy());
}

