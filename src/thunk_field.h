#ifndef THUNKFIELD_H
#define THUNKFIELD_H

#include "thunk_elems.h"
#include "thunk_fieldoffset.h"
#include "thunk_fieldsize.h"

/**
 * all thunks take parameters given by parent thunk type
 */
class ThunkField
{
public:
	ThunkField(
		ThunkType&		owner,
		ThunkFieldOffset*	in_off,
		ThunkFieldSize*		in_size,
		ThunkElements*		in_elems);
	virtual ~ThunkField();

	const ThunkFieldOffset* getOffset(void) const { return t_fieldoff; }
	const ThunkFieldSize* getSize(void) const { return t_size; }
	const ThunkElements* getElems(void) const { return t_elems; }
	ThunkField* copy(ThunkType& owner) const;

	Expr* copyNextOffset(void) const;

	const Type* getType(void) const { return t_size->getType(); }

	bool genProtos(void) const;
	bool genCode(void) const;

protected:
	ThunkField() {}
private:
	ThunkFieldOffset	*t_fieldoff;
	ThunkFieldSize		*t_size;
	ThunkElements		*t_elems;
};

#endif
