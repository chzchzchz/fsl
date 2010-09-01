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
	virtual const std::string getFCallName(void) const;
	ThunkFieldFunc(unsigned int i) : ThunkFunc(i) {}
	ThunkFieldFunc(Expr* e) : ThunkFunc(e) {}
	ThunkFieldFunc(void) {}

	void setPrefix(const std::string& p) { prefix = p; }

private:
	std::string		fieldname;
	std::string		prefix;
};

#endif
