#include "thunk_type.h"
#include "runtime_interface.h"
#include "thunk_field.h"

#include <iostream>

using namespace std;

extern RTInterface rt_glue;

ThunkType::~ThunkType(void)
{
	if (t_size != NULL) delete t_size;
	for (	thunkfunc_map::iterator	it = thunkfuncs_map.begin();
		it != thunkfuncs_map.end();
		it++)
	{
		delete (*it).second;
	}
}

bool ThunkType::registerFunc(const ThunkFunc* f)
{
	FCall		*fc;
	std::string	name;

	assert (f != NULL);

	fc = f->copyFCall();
	name = fc->getName();
	delete fc;

	if (thunkfuncs_map.count(name) != 0)
		return false;

	thunkfuncs_map[name] = f->copy();
	return true;
}

bool ThunkType::genCode(void) const
{
	for (	thunkfunc_map::const_iterator it = thunkfuncs_map.begin();
		it != thunkfuncs_map.end();
		it++)
	{
		ThunkFunc*	f;
		bool		ret;

		f = (*it).second;
		ret = f->genCode();
	}

	return true;
}

bool ThunkType::genProtos(void) const
{
	for (	thunkfunc_map::const_iterator it = thunkfuncs_map.begin();
		it != thunkfuncs_map.end();
		it++)
	{
		ThunkFunc*	f;
		bool		ret;

		f = (*it).second;
		ret = f->genProto();
	}

	return true;
}

ExprList* ThunkType::copyExprList(void) const
{
	assert (t_type != NULL);
	return new ExprList(rt_glue.getThunkClosure());
}

ThunkType* ThunkType::copy(void) const
{
	ThunkType	*ret;

	ret = new ThunkType(t_type);
	if (t_size != NULL) {
		ret->setSize(t_size->copy());
	}

	for (	thunkfunc_map::const_iterator it = thunkfuncs_map.begin();
		it != thunkfuncs_map.end();
		it++)
	{
		ret->registerFunc((*it).second);
	}

	ret->num_fields = num_fields;

	return ret;
}
