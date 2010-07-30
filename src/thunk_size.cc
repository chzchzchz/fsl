#include "thunk_type.h"
#include "thunk_size.h"
#include <iostream>

using namespace std;

const std::string ThunkSize::getFCallName(void) const
{
	return "__typesize_" + owner->getType()->getName(); 
}

FCall* ThunkSize::copyFCall(void) const
{
	return new FCall(new Id(getFCallName()), owner->copyExprList());
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

