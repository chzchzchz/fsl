#include "code_builder.h"
#include "thunk_fieldoffset.h"
#include "thunk_type.h"

extern CodeBuilder*	code_builder;

const std::string ThunkFieldOffset::getFCallName(void) const
{
	return "__thunkfieldoff_" + 
		owner->getType()->getName() + "_" +
		fieldname;
}

FCall* ThunkFieldOffset::copyFCall(void) const
{
	return new FCall(
		new Id(getFCallName()),
		owner->copyExprList());
}

ThunkFieldOffset* ThunkFieldOffset::copy(void) const
{
	assert (raw_expr != NULL);
	return new ThunkFieldOffset(
		owner, fieldname, raw_expr->copy());
}

