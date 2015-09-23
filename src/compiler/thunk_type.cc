#include "thunk_type.h"
#include "runtime_interface.h"
#include "thunk_field.h"

#include <iostream>

using namespace std;

extern RTInterface rt_glue;

ThunkType::~ThunkType(void)
{
	if (t_size != NULL) delete t_size;
	for (auto &p : thunkfuncs_map) delete p.second;
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
	for (const auto &p : thunkfuncs_map) {
		ThunkFunc*	f = p.second;
		bool		ret;
		ret = f->genCode();
		assert(ret);
	}

	return true;
}

bool ThunkType::genProtos(void) const
{
	for (const auto &p : thunkfuncs_map) {
		ThunkFunc*	f = p.second;
		bool		ret;
		ret = f->genProto();
		assert (ret);
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

	for (const auto &p : thunkfuncs_map)
		ret->registerFunc(p.second);

	ret->num_fields = num_fields;

	return ret;
}
