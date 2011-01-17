#ifndef DETACHEDPREAMBLE_H
#define DETACHEDPREAMBLE_H

#include "AST.h"
#include "type.h"
#include "expr.h"

#include <map>
#include <assert.h>

class Preamble;

class DetachedPreamble : public GlobalStmt
{
public:
	DetachedPreamble(
		Id* in_type_name,
		Preamble* in_preamble)
	:  type_name(in_type_name), preamble(in_preamble)
	{
		assert (type_name != NULL);
		assert (preamble != NULL);
	}

	virtual ~DetachedPreamble()
	{
		delete type_name;
		delete preamble;
	}

	virtual void print(std::ostream& out) const
		{out << "DETACHED PREAMBLE" << std::endl; }

	const std::string& getTypeName(void) const
	{
		return type_name->getName();
	}

	Preamble* getPreamble(void) const
	{
		return preamble;
	}

private:
	DetachedPreamble() {}
	Id		*type_name;
	Preamble	*preamble;
};

#endif
