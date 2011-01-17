#ifndef DEFTYPE_H
#define DEFTYPE_H

#include "AST.h"
#include "expr.h"

#include <map>
#include <assert.h>

typedef std::map<std::string, class DefType*>	deftype_map;


class DefType : public GlobalStmt
{
public:
	DefType(Id* in_ref_name, Id* in_ref_target)
	: ref_name(in_ref_name), ref_target(in_ref_target)
	{
		assert (ref_name != NULL);
		assert (ref_target != NULL);
		assert (getTargetTypeName() != getName());
	}

	virtual ~DefType()
	{
		delete ref_name;
		delete ref_target;
	}

	virtual void print(std::ostream& out) const
		{out << "DEFTYPE" << std::endl; }

	virtual const std::string& getName(void) const
	{
		return ref_name->getName();
	}

	virtual const std::string& getTargetTypeName(void) const
	{
		return ref_target->getName();
	}

private:
	DefType() {}
	Id*	ref_name;
	Id*	ref_target;
};

#endif
