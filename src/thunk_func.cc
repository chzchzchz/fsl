#include "code_builder.h"
#include "thunk_type.h"
#include "thunk_func.h"

#include <iostream>

extern CodeBuilder	*code_builder;

using namespace std;

bool ThunkFunc::genProto(void) const
{
	/* return uint64_t by default */
	code_builder->genThunkProto(getFCallName());
	return true;
}

bool ThunkFunc::genCode(void) const
{
	assert (owner != NULL);
	code_builder->genCode(owner->getType(), getFCallName(), raw_expr);
	return true;
}

void ThunkFunc::setOwner(const ThunkType* tt)
{
	if (tt == owner)
		return;

	assert (tt != NULL);

	owner = tt;
}

