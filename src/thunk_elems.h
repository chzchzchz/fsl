#ifndef THUNKELEMS_H
#define THUNKELEMS_H

#include <string>
#include <assert.h>

#include "thunk_func.h"

/*
 * describes how many elements there are for a given materialization taking an 
 * input of the materialization's base offset
 */
class ThunkElements : public ThunkFunc
{
public:
	ThunkElements(
		const ThunkType* in_owner, 
		const std::string& in_fieldname,
		unsigned int i)
	: ThunkFunc(in_owner, i),
	  fieldname(in_fieldname) {}

	ThunkElements(
		const ThunkType* in_owner, 
		const std::string& in_fieldname,
		Expr* e)
	: ThunkFunc(in_owner, e),
	  fieldname(in_fieldname) {}

	virtual ~ThunkElements() {}

	FCall* copyFCall(void) const;

	virtual ThunkElements* copy(void) const;
	bool isSingleton(void) const;

protected:
	const std::string getFCallName(void) const;

	std::string	fieldname;
};


#endif
