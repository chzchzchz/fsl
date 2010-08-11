#ifndef PREAMBLE_H
#define PREAMBLE_H

#include "collection.h"

class Preamble
{
public:
	Preamble(FCall* in_fc, PtrList<Id>* in_when_ids = NULL);
	virtual ~Preamble();

	const FCall* getFCall(void) const
	{
		return fc;
	}

	const PtrList<Id>* getWhenList(void) const
	{
		return when_ids;
	}
private:
	FCall		*fc;
	PtrList<Id>	*when_ids;
};

#endif
