#ifndef THUNK_FIELDFUNC_H
#define THUNK_FIELDFUNC_H

#include <string>

#include "thunk_func.h"

class ThunkFieldFunc : public ThunkFunc 
{
public:
	virtual ~ThunkFieldFunc() {}

	void setFieldName(const std::string& s) 
	{
		assert (s.length() > 0 && "Expect non-empty string");
		fieldname = s;	
	}

	const std::string& getFieldName(void) const
	{
		assert (fieldname.length() > 0);
		return fieldname;
	}

protected:
	ThunkFieldFunc(unsigned int i) : ThunkFunc(i) {}
	ThunkFieldFunc(Expr* e) : ThunkFunc(e) {}
	ThunkFieldFunc(void) {}

private:
	std::string		fieldname;
};

#endif
