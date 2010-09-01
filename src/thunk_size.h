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
	ThunkSize(unsigned int size_bits)
	: ThunkFunc(size_bits), t(NULL) {}

	ThunkSize(Expr* e)
	: ThunkFunc(e), t(NULL) {}

	virtual ~ThunkSize() {}

	Expr* copyConstValue(void) const;

	ThunkSize* copy(void) const;

protected:
	const class Type	*t;

private:
	const std::string getFCallName(void) const;
};

#endif
