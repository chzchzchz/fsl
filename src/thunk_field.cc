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
	} else if (t_elems->isFixed() == true) {
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

	/* XXX */
	return Expr::rewriteReplace(
		ret,
		rt_glue.getThunkArgIdx(),
		new Number(0));
}

ThunkField* ThunkField::copy(ThunkType& new_owner) const
{
	return new ThunkField(
		new_owner, 
		fieldname, 
		t_fieldoff->copy(),
		t_size->copy(),
		t_elems->copy(),
		t_params->copy(),
		field_num);
}

ThunkField* ThunkField::createInvisible(
	ThunkType		&owner,
	const std::string	&in_fieldname,
	ThunkFieldOffset*	in_off,
	ThunkFieldSize*		in_size)
{
	return new ThunkField(
		owner,
		in_fieldname,
		in_off,
		in_size,
		new ThunkElements(1),
		ThunkParams::createNoParams(),
		TF_FIELDNUM_NONE);
}


ThunkField::ThunkField(
	ThunkType&		in_owner,
	const std::string	&in_fieldname,
	ThunkFieldOffset*	in_off,
	ThunkFieldSize*		in_size,
	ThunkElements*		in_elems,
	ThunkParams*		in_params,
	unsigned int		in_fieldnum)
: fieldname(in_fieldname),
  t_fieldoff(in_off), t_size(in_size), t_elems(in_elems), t_params(in_params),
  field_num(in_fieldnum),
  owner_type(in_owner.getType())
{
	assert (t_fieldoff != NULL);
	assert (t_elems != NULL);
	assert (t_size != NULL);
	assert (t_params != NULL);
	assert (field_num == TF_FIELDNUM_NONE ||
		(field_num < in_owner.getNumFields() && "Bad type in copy"));

	setFields(in_owner);
}

ThunkField::ThunkField(
	ThunkType&		in_owner,
	const std::string	&in_fieldname,
	ThunkFieldOffset*	in_off,
	ThunkFieldSize*		in_size,
	ThunkElements*		in_elems,
	ThunkParams*		in_params)
: fieldname(in_fieldname),
  t_fieldoff(in_off), t_size(in_size), t_elems(in_elems), t_params(in_params),
  owner_type(in_owner.getType())
{
	assert (t_fieldoff != NULL);
	assert (t_elems != NULL);
	assert (t_size != NULL);
	assert (t_params != NULL);

	setFields(in_owner);

	field_num = in_owner.incNumFields();

	cout << "Owner Type: " << owner_type->getName() << endl;
	cout << "Fieldname: " << fieldname << endl;
	cout << "Fieldnum: " << field_num << endl;
	cout << "-------" << endl;
}

void ThunkField::setFields(ThunkType& in_owner)
{
	t_fieldoff->setOwner(&in_owner);
	t_elems->setOwner(&in_owner);
	t_params->setOwner(&in_owner);
	t_size->setOwner(&in_owner);

	t_fieldoff->setFieldName(fieldname);
	t_elems->setFieldName(fieldname);
	t_size->setFieldName(fieldname);

	t_params->setFieldName(fieldname);
	t_params->setFieldType(t_size->getType());

	in_owner.registerFunc(t_fieldoff);
	in_owner.registerFunc(t_size);
	in_owner.registerFunc(t_elems);
	in_owner.registerFunc(t_params);

	assert (t_params->getFieldType() == t_size->getType());
}

ThunkField::~ThunkField() 
{
	delete t_fieldoff;
	delete t_size;
	delete t_elems;
	delete t_params;
}

