#include "AST.h"
#include "type.h"
#include "func_args.h"

using namespace std;

extern type_map types_map;

FuncArgs::FuncArgs(const ArgsList* in_args)
{
	assert (in_args != NULL);
	args = in_args->copy();

	for (unsigned int i = 0; i < args->size(); i++) {
		pair<Id*, Id*>	p(args->get(i));

		args_map[p.second->getName()] = p.first->getName();
	}
}

FuncArgs::~FuncArgs(void)
{
	delete args;
}

bool FuncArgs::lookupType(const std::string& name, std::string& field) const
{
	funcarg_map::const_iterator it;

	it = args_map.find(name);
	if (it == args_map.end())
		return false;

	field = (*it).second;
	return true;
}

bool FuncArgs::hasField(const std::string& name) const
{
	return (args_map.count(name) != 0);
}


const Type* FuncArgs::getType(const std::string& name) const
{
	string	type_name;
	if (lookupType(name, type_name) == false)
		return NULL;
	if (types_map.count(type_name) == 0)
		return NULL;
	return (*(types_map.find(type_name))).second;
}

