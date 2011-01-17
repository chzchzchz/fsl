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
	ThunkElements(unsigned int i, bool in_nofollow = false)
	: ThunkFieldFunc(i), fixed(false), nofollow(in_nofollow)
	{ setPrefix("thunkelems"); }

	ThunkElements(Expr* e, bool in_fixed = false)
	: ThunkFieldFunc(e), fixed(in_fixed) { setPrefix("thunkelems"); }

	ThunkElements(Expr* e, bool in_fixed, bool in_nofollow)
	: ThunkFieldFunc(e), fixed(in_fixed), nofollow(in_nofollow)
	{ setPrefix("thunkelems"); }

	virtual ~ThunkElements() {}
	virtual ThunkElements* copy(void) const;

	bool isSingleton(void) const;
	bool isFixed() const { return fixed; }
	bool isNoFollow(void) const { return nofollow; }
private:
	bool fixed; /* array, all elems same size */
	bool nofollow;
};

#endif
