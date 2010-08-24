#include "thunk_field.h"
#include "thunk_type.h"

#include "runtime_interface.h"

using namespace std;

extern RTInterface	rt_glue;

Expr* ThunkField::copyNextOffset(void) const
{
	Expr	*ret;

	assert (t_elems != NULL);

	if (t_elems->isSingleton() == true) {
		ret = new AOPAdd(
			t_fieldoff->copyFCall(),
			t_size->copyFCall());
	} else if (t_size->isConstant()) {
		ret = new AOPAdd(
			t_fieldoff->copyFCall(),
			new AOPMul(
				t_size->copyFCall(),
				t_elems->copyFCall()));
	} else {
		ret = new AOPAdd(
			t_fieldoff->copyFCall(),
			rt_glue.computeArrayBits(this));
	}

	return ret;
}

ThunkField* ThunkField::copy(ThunkType& new_owner) const
{
	ThunkFieldOffset	*toff_copy;
	ThunkFieldSize		*tsize_copy;
	ThunkElements		*telems_copy;
	ThunkParams		*tparams_copy;

	toff_copy = t_fieldoff->copy();
	tsize_copy = t_size->copy();
	telems_copy = t_elems->copy();
	tparams_copy = t_params->copy();

	return new ThunkField(
		new_owner, 
		fieldname, 
		toff_copy, tsize_copy, telems_copy, tparams_copy);
}


ThunkField::ThunkField(
	ThunkType&		in_owner,
	const std::string	&in_fieldname,
	ThunkFieldOffset*	in_off,
	ThunkFieldSize*		in_size,
	ThunkElements*		in_elems,
	ThunkParams*		in_params)
: fieldname(in_fieldname),
  t_fieldoff(in_off), t_size(in_size), t_elems(in_elems), t_params(in_params)
{
	assert (t_fieldoff != NULL);
	assert (t_elems != NULL);
	assert (t_size != NULL);
	assert (t_params != NULL);

	t_fieldoff->setOwner(&in_owner);
	t_elems->setOwner(&in_owner);
	t_params->setOwner(&in_owner);
	t_size->setOwner(&in_owner);

	t_fieldoff->setFieldName(fieldname);
	t_elems->setFieldName(fieldname);
	t_params->setFieldName(fieldname);
	t_size->setFieldName(fieldname);

	in_owner.registerFunc(t_fieldoff);
	in_owner.registerFunc(t_size);
	in_owner.registerFunc(t_elems);
	in_owner.registerFunc(t_params);
}

ThunkField::~ThunkField() 
{
	delete t_fieldoff;
	delete t_size;
	delete t_elems;
	delete t_params;
}

