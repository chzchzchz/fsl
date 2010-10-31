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
	: ThunkFieldFunc(i), fixed(false) { setPrefix("thunkelems"); }

	ThunkElements(Expr* e, bool in_fixed = false)
	: ThunkFieldFunc(e), fixed(in_fixed) { setPrefix("thunkelems"); }

	virtual ~ThunkElements() {}

	virtual ThunkElements* copy(void) const;

	bool isSingleton(void) const;
	/* array, all elems same size */
	bool isFixed() const { return fixed; }

protected:
private:
	bool fixed;
};


#endif
