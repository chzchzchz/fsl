#include "thunk_type.h"
#include <iostream>

using namespace std;

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
	const ArgsList*	args;
	ExprList*	ret;

	assert (t_type != NULL);

	ret = new ExprList();
	ret->add(new Id("__thunk_arg_off"));

	args = t_type->getArgs();
	if (args != NULL) {
		for (unsigned int i = 0; i < args->size(); i++) {
			ret->add(args->get(i).second->copy());
		}
	}

	return ret;
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

	return ret;
}

