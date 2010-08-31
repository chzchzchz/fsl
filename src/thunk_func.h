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

	void setOwner(const ThunkType* tt);
	const ThunkType* getOwner(void) const 
	{
		assert (owner != NULL  && "Owner not set");
		return owner; 
	}

private:
	const ThunkType*	owner;

protected:
	ThunkFunc(unsigned int i) : owner(NULL)
	{
		raw_expr = new Number(i);
	}

	ThunkFunc(Expr* e) : owner(NULL)
	{
		assert (e != NULL);
		raw_expr = e->simplify();
		delete e;
	}

	ThunkFunc(void)
	: owner(NULL), raw_expr(NULL) {}


	Expr*			raw_expr;
	virtual const std::string getFCallName(void) const = 0;

};

#endif
