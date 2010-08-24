#include "code_builder.h"
#include "thunk_fieldoffset.h"
#include "thunk_type.h"

extern CodeBuilder*	code_builder;

const std::string ThunkFieldOffset::getFCallName(void) const
{
	return "__thunkfieldoff_" + 
		getOwner()->getType()->getName() + "_" +
		getFieldName();
}

FCall* ThunkFieldOffset::copyFCall(void) const
{
	return new FCall(
		new Id(getFCallName()),
		getOwner()->copyExprList());
}

ThunkFieldOffset* ThunkFieldOffset::copy(void) const
{
	ThunkFieldOffset	*ret;

	assert (raw_expr != NULL);

	ret = new ThunkFieldOffset(raw_expr->copy());
	ret->setFieldName(getFieldName());
	ret->setOwner(getOwner());

	return ret;
}
