#ifndef THUNKSIZE_H
#define THUNKSIZE_H

#include <assert.h>
#include "thunk_func.h"

/**
 * represents a function that takes an offset pointing to some materialization
 * of a type and that materialization's size
 *
 * NOTE: actual type size may rely on parameters... this is treated as if 
 * it has already had all of its parameters passed to it
 */
class ThunkSize : public ThunkFunc
{
public:
	ThunkSize(const ThunkType* in_owner, unsigned int size_bits)
	: ThunkFunc(in_owner, size_bits), t(NULL) {}

	ThunkSize(const ThunkType* in_owner, Expr* e)
	: ThunkFunc(in_owner, e), t(NULL) {}

	virtual ~ThunkSize() {}

	virtual FCall* copyFCall(void) const;
	Expr* copyConstValue(void) const;


	ThunkSize* copy(void) const
	{
		return new ThunkSize(owner, raw_expr->copy());
	}

protected:
	const class Type	*t;

private:
	const std::string getFCallName(void) const;
};

#endif
