#ifndef THUNKFIELDOFFSET_H
#define THUNKFIELDOFFSET_H

#include <string>
#include <assert.h>

#include "thunk_func.h"

/* XXX!!! add functions to get data for
 * fully qualified type (e.g. convert 
 * parent params into FCall params) */

/* represents the offset of a type materialization's given the offset to 
 * the parent class
 */
class ThunkFieldOffset : public ThunkFunc
{
public:
	ThunkFieldOffset(
		const ThunkType*	in_owner, 
		const std::string	&in_fieldname,
		unsigned int		off_bits)
	: ThunkFunc(in_owner, off_bits),
	  fieldname(in_fieldname) {}

	ThunkFieldOffset(
		const ThunkType*	in_owner,
		const std::string	&in_fieldname,
		Expr*			e)
	: ThunkFunc(in_owner, e),
	  fieldname(in_fieldname) {}

	virtual ~ThunkFieldOffset() {}

	FCall* copyFCall(void) const;

	virtual ThunkFieldOffset* copy(void) const;
protected:
	ThunkFieldOffset(
		const ThunkType* 	in_owner,
		const std::string	&in_fieldname)
	: ThunkFunc(in_owner),
	  fieldname(in_fieldname) {}

	const std::string getFCallName(void) const;
	const std::string fieldname;
};

#endif
