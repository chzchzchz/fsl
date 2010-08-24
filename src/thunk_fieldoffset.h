#ifndef THUNKFIELDOFFSET_H
#define THUNKFIELDOFFSET_H

#include <string>
#include <assert.h>

#include "thunk_fieldfunc.h"

/* XXX!!! add functions to get data for
 * fully qualified type (e.g. convert 
 * parent params into FCall params) */

/* represents the offset of a type materialization's given the offset to 
 * the parent class
 */
class ThunkFieldOffset : public ThunkFieldFunc
{
public:
	ThunkFieldOffset(unsigned int off_bits)
	: ThunkFieldFunc(off_bits) {}

	ThunkFieldOffset(Expr* e)
	: ThunkFieldFunc(e) {}

	virtual ~ThunkFieldOffset() {}

	virtual FCall* copyFCall(void) const;
	virtual ThunkFieldOffset* copy(void) const;

protected:
	ThunkFieldOffset() {}

	const std::string getFCallName(void) const;
};

#endif
