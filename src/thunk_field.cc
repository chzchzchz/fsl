#include "thunk_field.h"
#include "thunk_type.h"

using namespace std;

Expr* ThunkField::copyNextOffset(void) const
{
	Expr	*ret;

	assert (t_elems != NULL);

	if (t_elems->isSingleton() == true) {
		ret = new AOPAdd(
			t_fieldoff->copyFCall(),
			t_size->copyFCall());
	} else {
		ret = new AOPAdd(
			t_fieldoff->copyFCall(),
			new AOPMul(
				t_size->copyFCall(),
				t_elems->copyFCall()));
	}

	return ret;
}

ThunkField* ThunkField::copy(ThunkType& owner) const
{
	return new ThunkField(
		owner, 
		t_fieldoff->copy(),
		t_size->copy(),
		t_elems->copy());
}

ThunkField::ThunkField(
	ThunkType&		owner,
	ThunkFieldOffset*	in_off,
	ThunkFieldSize*		in_size,
	ThunkElements*		in_elems)
: t_fieldoff(in_off), t_size(in_size), t_elems(in_elems)
{
	assert (t_fieldoff != NULL);
	assert (t_elems != NULL);
	assert (t_size != NULL);
	owner.registerFunc(t_fieldoff);
	owner.registerFunc(t_size);
	owner.registerFunc(t_elems);
}

ThunkField::~ThunkField() 
{
	delete t_fieldoff;
	delete t_size;
	delete t_elems;
}
