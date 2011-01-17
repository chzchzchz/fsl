#include "thunk_type.h"
#include "thunk_size.h"
#include <iostream>

using namespace std;

const std::string ThunkSize::getFCallName(void) const
{
	const Type	*owner_t;
	const ThunkType	*tt;

	tt = getOwner();
	assert (tt != NULL);

	owner_t = tt->getType();
	assert (owner_t != NULL);

	return "__typesize_" + owner_t->getName();
}

ThunkSize* ThunkSize::copy(void) const
{
	ThunkSize	*ret;

	ret = new ThunkSize(raw_expr->copy());
	ret->setOwner(getOwner());

	return ret;
}

Expr* ThunkSize::copyConstValue(void) const
{
	if (dynamic_cast<const Number*>(raw_expr) == NULL) {
		cerr << "ThunkSize: expected constant, got ";
		raw_expr->print(cerr);
		cerr << endl;
		assert (0 == 1);
	}
	return raw_expr->copy();
}
