#include <assert.h>

#include "expr.h"
#include "preamble.h"

Preamble::Preamble(
	Id* in_name, preamble_args* in_args,
	Id* in_as_name,
	PtrList<Id>* in_when_ids)
: name(in_name),
  args(in_args),
  when_ids(in_when_ids),
  as_name(in_as_name)
{
	assert (name != NULL);
}

Preamble::Preamble(Id* in_name, Id* in_as_name, PtrList<Id>* in_when_ids)
: name(in_name),
  args(NULL),
  when_ids(in_when_ids),
  as_name(in_as_name)
{
	assert (name != NULL);
}


Preamble::~Preamble()
{
	delete name;
	if (args != NULL) delete args;
	if (when_ids != NULL) delete when_ids;
	if (as_name != NULL) delete as_name;
}
