#ifndef PREAMBLE_H
#define PREAMBLE_H

#include "collection.h"

typedef PtrList<CondOrExpr> preamble_args;
typedef std::list<const class Preamble*> preamble_list;

class Preamble
{
public:
	Preamble(
		Id* in_name, 
		preamble_args* in_args,
		PtrList<Id>* in_when_ids = NULL);

	Preamble(
		Id* in_name, 
		PtrList<Id>* in_when_ids = NULL);

	virtual ~Preamble();

	const std::string& getName() const
	{
		return name->getName();
	}

	const preamble_args* getArgsList(void) const
	{
		return args;
	}

	const PtrList<Id>* getWhenList(void) const
	{
		return when_ids;
	}

private:
	Id		*name;
	preamble_args	*args;
	PtrList<Id>	*when_ids;
};

#endif
