#include <assert.h>

#include "expr.h"
#include "preamble.h"


Preamble::Preamble(FCall* in_fc, PtrList<Id>* in_when_ids)
:	fc(in_fc), 
	when_ids(in_when_ids)
{
	assert(fc != NULL);
}

Preamble::~Preamble()
{
	delete fc;
	if (when_ids != NULL) delete when_ids;
}

