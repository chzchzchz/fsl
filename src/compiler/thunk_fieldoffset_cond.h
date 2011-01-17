#ifndef THUNK_FO_COND_H
#define THUNK_FO_COND_H

#include "thunk_fieldoffset.h"

/* like a field offset.. but with a conditional! */
class ThunkFieldOffsetCond : public ThunkFieldOffset
{
public:
	ThunkFieldOffsetCond(
		CondExpr*		in_cond,
		Expr*			in_true_off,
		Expr*			in_false_off = NULL)
	: cond_expr(in_cond),
	  true_off(in_true_off),
	  false_off(in_false_off)
	{
		assert (cond_expr != NULL);
		assert (true_off != NULL);
	}

	virtual ~ThunkFieldOffsetCond();
	virtual ThunkFieldOffsetCond* copy(void) const;

	virtual bool genProto(void) const;
	virtual bool genCode(void) const;

	/* gets the unchecked call */
	FCall* copyFCallUnchecked(void) const;
	FCall* copyFCallPresent(void) const;

	bool isAlwaysPresent(void) const
	{
		return (false_off != NULL);
	}
private:
	const std::string getPresentFCallName(void) const;
	CondExpr	*cond_expr;
	Expr		*true_off;
	Expr		*false_off;
};

#endif
