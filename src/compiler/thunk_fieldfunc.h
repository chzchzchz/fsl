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

	void setFieldType(const Type* t) { fieldtype = t; fieldtype_was_set = true; }
	const Type* getFieldType(void) const
	{
		assert (fieldtype_was_set);
		return fieldtype;
	}
protected:
	virtual const std::string getFCallName(void) const;
	ThunkFieldFunc(unsigned int i)
		: ThunkFunc(i), fieldtype_was_set(false), fieldtype(NULL) {}
	ThunkFieldFunc(Expr* e)
		: ThunkFunc(e) , fieldtype_was_set(false), fieldtype(NULL) {}
	ThunkFieldFunc(void)
		: fieldtype_was_set(false), fieldtype(NULL) {}

	void setPrefix(const std::string& p) { prefix = p; }

private:
	bool			fieldtype_was_set;
	const Type		*fieldtype;
	std::string		fieldname;
	std::string		prefix;
};

#endif
