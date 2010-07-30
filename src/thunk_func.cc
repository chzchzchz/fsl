#include "code_builder.h"
#include "thunk_type.h"
#include "thunk_func.h"

extern CodeBuilder	*code_builder;

bool ThunkFunc::genProto(void) const
{
	code_builder->genProto(getFCallName(), owner->getThunkArgCount());
	return true;
}

bool ThunkFunc::genCode(void) const
{
	code_builder->genCode(owner->getType(), getFCallName(), raw_expr);
	return true;
}

