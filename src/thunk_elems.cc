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

ThunkElements* ThunkElements::copy(void) const
{
	ThunkElements*	ret;

	assert (raw_expr != NULL);

	ret = new ThunkElements(raw_expr->copy());
	ret->setOwner(getOwner());
	ret->setFieldName(getFieldName());

	return ret;
}

