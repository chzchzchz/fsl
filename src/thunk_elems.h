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
	: ThunkFieldFunc(i) { setPrefix("thunkelems"); }

	ThunkElements(Expr* e)
	: ThunkFieldFunc(e) { setPrefix("thunkelems"); }

	virtual ~ThunkElements() {}

	virtual ThunkElements* copy(void) const;

	bool isSingleton(void) const;

protected:
};


#endif
