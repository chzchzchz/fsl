#ifndef THUNKFIELDSIZE_H
#define THUNKFIELDSIZE_H

#include <string>
#include "thunk_func.h"

class Type;

class ThunkFieldSize : public ThunkFunc
{
public:
	ThunkFieldSize(
		const ThunkType* in_owner, 
		const std::string& in_fieldname,
		unsigned int size_bits)
	: ThunkFunc(in_owner, size_bits), 
	  tf_owner(NULL),
	  t(NULL),
	  fieldname(in_fieldname) {}

	ThunkFieldSize(
		const ThunkType* in_owner,
		const std::string& in_fieldname,
		Expr* e)
	: ThunkFunc(in_owner, e),
	  tf_owner(NULL),
	  t(NULL),
	  fieldname(in_fieldname) {}

	ThunkFieldSize(
		const ThunkType* in_owner, 
		const std::string& in_fieldname,
		const Type* t_in)
	: ThunkFunc(in_owner),
	  tf_owner(NULL),
	  t(t_in),
	  fieldname(in_fieldname)
	{
		assert (t != NULL);
	}

	virtual ~ThunkFieldSize() {}

	virtual FCall* copyFCall(void) const;

	virtual ThunkFieldSize* copy(void) const;
	Expr* copyConstValue(void) const;
	bool genCode(void) const;
	const Type* getType(void) const { return t; }

	void setThunkField(class ThunkField* tf) { tf_owner = tf; }
protected:
	const std::string getFCallName(void) const;

	const class ThunkField	*tf_owner;
	const Type		*t;
	const std::string	fieldname;

};


#endif
