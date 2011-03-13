#include "thunk_field.h"
#include "thunk_type.h"
#include "table_gen.h"
#include "struct_writer.h"
#include "thunk_fieldoffset_cond.h"

#include "runtime_interface.h"

using namespace std;

extern RTInterface	rt_glue;

Expr* ThunkField::copyNumBits(void) const
{
	if (t_elems->isSingleton() == true) {
		return t_size->copyFCall();
	} else if (t_size->isConstant() || getType()->isUnion()) {
		return new AOPMul(t_size->copyFCall(), t_elems->copyFCall());
	} else if (t_elems->isFixed() == true) {
		return new AOPMul(t_size->copyFCall(), t_elems->copyFCall());
	} else {
		return rt_glue.computeArrayBits(this);
	}

	assert (0 == 1 && "Don't know how to compute num bits");
	return NULL;
}

Expr* ThunkField::copyNextOffset(void) const
{
	Expr	*ret;

	assert (t_elems != NULL);
	ret = copyNumBits();
	ret = new AOPAdd(t_fieldoff->copyFCall(), ret);

	/* XXX */
	return Expr::rewriteReplace(
		ret, rt_glue.getThunkArgIdx(), new Number(0));
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

void ThunkField::genFieldEntry(TableGen* tg) const
{
	StructWriter			sw(tg->getOS());
	const Type			*field_type;
	const ThunkFieldOffsetCond	*condoff;
	FCall				*field_off_fc;
	FCall				*field_elems_fc;
	FCall				*field_size_fc;
	FCall				*field_params_fc;

	condoff = dynamic_cast<const ThunkFieldOffsetCond*>(getOffset());
	field_off_fc = getOffset()->copyFCall();
	field_elems_fc = getElems()->copyFCall();
	field_size_fc = getSize()->copyFCall();
	field_params_fc = getParams()->copyFCall();
	field_type = getType();

	sw.writeStr("tf_fieldname", getFieldName());
	sw.write("tf_fieldbitoff", field_off_fc->getName());
	if (field_type != NULL)
		sw.write32("tf_typenum", field_type->getTypeNum());
	else
		sw.write32("tf_typenum", ~0);
	sw.write("tf_elemcount", field_elems_fc->getName());
	sw.write("tf_typesize", field_size_fc->getName());
	sw.write("tf_params",  field_params_fc->getName());

	if (condoff != NULL) {
		FCall	*present_fc;
		present_fc = condoff->copyFCallPresent();
		sw.write("tf_cond", present_fc->getName());
		delete present_fc;
	} else
		sw.write("tf_cond", "NULL");

	sw.write("tf_flags",
		((getElems()->isNoFollow() == true) ? 4 : 0) |
		((getElems()->isFixed() == true) ? 2 : 0) |
		((getSize()->isConstant() == true) ? 1 : 0));

	sw.write("tf_fieldnum", getFieldNum());

	delete field_params_fc;
	delete field_off_fc;
	delete field_elems_fc;
	delete field_size_fc;
}

void ThunkField::genFieldExtern(TableGen* tg) const
{
	ostream				&out(tg->getOS());
	const ThunkFieldOffsetCond	*fo_cond;
	FCall				*fc_off;
	FCall				*fc_elems;
	FCall				*fc_size;

	fc_off = getOffset()->copyFCall();
	fc_elems = getElems()->copyFCall();
	fc_size = getSize()->copyFCall();

	tg->printExternFuncThunk(fc_off->getName());
	tg->printExternFuncThunk(fc_elems->getName());
	tg->printExternFuncThunk(fc_size->getName());
	tg->printExternFuncThunkParams(getParams());

	fo_cond = dynamic_cast<const ThunkFieldOffsetCond*>(getOffset());
	if (fo_cond != NULL) {
		FCall	*cond_fc;
		cond_fc = fo_cond->copyFCallPresent();
		tg->printExternFuncThunk(cond_fc->getName(), "bool");
		delete cond_fc;
	}

	out << endl;

	delete fc_off;
	delete fc_elems;
	delete fc_size;
}
