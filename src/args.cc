#include "typeclosure.h"
#include "type.h"
#include "args.h"

using namespace std;

extern type_map	types_map;

ArgsList::~ArgsList()
{
	for (unsigned int i = 0; i < types.size(); i++) {
		delete types[i];
		delete names[i];
	}
}

void ArgsList::add(Id* type_name, Id* arg_name)
{
	types.push_back(type_name);
	names.push_back(arg_name);
	name_to_type[arg_name->getName()] = type_name->getName();
}

bool ArgsList::lookupType(const string& name, string& type_ret) const
{
	arg_map::const_iterator it;

	it = name_to_type.find(name);
	if (it == name_to_type.end())
		return false;

	type_ret = (*it).second;
	return true;
}

arg_elem ArgsList::get(unsigned int i) const
{
	return arg_elem(types[i], names[i]);
}

bool ArgsList::hasField(const string& name) const
{
	return (name_to_type.count(name) != 0);
}

const Type* ArgsList::getType(const string& name) const
{
	string	type_name;
	if (lookupType(name, type_name) == false)
		return NULL;
	if (types_map.count(type_name) == 0)
		return NULL;
	return (*(types_map.find(type_name))).second;
}

ArgsList* ArgsList::copy(void) const
{
	ArgsList*	ret = new ArgsList();

	for (unsigned int i = 0; i < size(); i++) {
		pair<Id*, Id*>	p(get(i));
		ret->add(p.first->copy(), p.second->copy());
	}

	return ret;
}

/* number of entries in parambuf to store everything */
unsigned int ArgsList::getNumParamBufEntries(void) const
{
	unsigned int		ret;

	ret = 0;
	for (unsigned int i = 0; i < names.size(); i++) {
		const Type	*t;
		t = getType(names[i]->getName());
		if (t != NULL) {
			ret += t->getParamBufEntryCount();
			ret += RT_CLO_ELEMS-1; /* store offset+xlate */
		} else
			ret++;
	}

	return ret;
}
