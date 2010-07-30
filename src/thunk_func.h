#ifndef THUNKFUNC_H
#define THUNKFUNC_H

#include <assert.h>
#include "expr.h"

class ThunkType;

class ThunkFunc
{
public:
	virtual ThunkFunc* copy(void) const = 0;
	virtual FCall* copyFCall(void) const = 0;
	virtual ~ThunkFunc() 
	{
		if (raw_expr != NULL) delete raw_expr;	
	}

	virtual bool genProto(void) const;
	virtual bool genCode(void) const;

protected:
	ThunkFunc(const ThunkType* in_owner, unsigned int i)
	: owner(in_owner)
	{
		assert (owner != NULL);
		raw_expr = new Number(i);
	}

	ThunkFunc(const ThunkType* in_owner, Expr* e)
	: owner(in_owner)
	{
		assert (owner != NULL);
		assert (e != NULL);

		raw_expr = e->simplify();
		delete e;
	}

	ThunkFunc(const ThunkType* in_owner)
	: owner(in_owner),
	  raw_expr(NULL) {}

	const ThunkType*	owner;
	Expr*			raw_expr;
	virtual const std::string getFCallName(void) const = 0;
private:
	ThunkFunc() {}
};

#endif
