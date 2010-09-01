#include "code_builder.h"
#include "thunk_fieldoffset.h"
#include "thunk_type.h"

extern CodeBuilder*	code_builder;

ThunkFieldOffset* ThunkFieldOffset::copy(void) const
{
	ThunkFieldOffset	*ret;

	assert (raw_expr != NULL);

	ret = new ThunkFieldOffset(raw_expr->copy());
	ret->setFieldName(getFieldName());
	ret->setOwner(getOwner());

	return ret;
}
