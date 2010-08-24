#ifndef THUNKELEMS_H
#define THUNKELEMS_H

#include <string>
#include <assert.h>

#include "thunk_fieldfunc.h"

/*
 * describes how many elements there are for a given materialization taking an 
 * input of the materialization's base offset
 */
class ThunkElements : public ThunkFieldFunc
{
public:
	ThunkElements(unsigned int i)
	: ThunkFieldFunc(i) {}

	ThunkElements(Expr* e)
	: ThunkFieldFunc(e) {}

	virtual ~ThunkElements() {}

	virtual FCall* copyFCall(void) const;
	virtual ThunkElements* copy(void) const;

	bool isSingleton(void) const;

protected:
	const std::string getFCallName(void) const;
};


#endif
