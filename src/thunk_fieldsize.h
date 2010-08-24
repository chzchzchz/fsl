#ifndef THUNKFIELDSIZE_H
#define THUNKFIELDSIZE_H

#include <string>
#include "thunk_fieldfunc.h"

class Type;

class ThunkFieldSize : public ThunkFieldFunc
{
public:
	ThunkFieldSize(unsigned int size_bits)
	: ThunkFieldFunc(size_bits), t(NULL) {}

	ThunkFieldSize(Expr* e)
	: ThunkFieldFunc(e), t(NULL) {}

	ThunkFieldSize(const Type* t_in)
	: t(t_in) 
	{
		assert (t != NULL); 
	}

	virtual ~ThunkFieldSize() {}

	virtual FCall* copyFCall(void) const;
	virtual ThunkFieldSize* copy(void) const;
	virtual bool genCode(void) const;

	Expr* copyConstValue(void) const;

	const Type* getType(void) const { return t; }

	bool isConstant(void) const
	{
		if (getType() == NULL)
			return true;

		/* TODO-- need to be smarter here */
		return false;
	}

protected:
	const std::string getFCallName(void) const;
	const Type		*t;
};


#endif
