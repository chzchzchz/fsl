#include "AST.h"
#include "func_args.h"

using namespace std;

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
