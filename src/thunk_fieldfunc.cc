#include "thunk_fieldfunc.h"
#include "thunk_type.h"

const std::string ThunkFieldFunc::getFCallName(void) const
{
	assert (prefix.size() > 0);
	return "__" + prefix + "_" +
		getOwner()->getType()->getName() + "_" +
		getFieldName();
}
