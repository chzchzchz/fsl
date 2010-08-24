#ifndef THUNKTYPE_H
#define THUNKTYPE_H

#include <string>
#include <map>
#include <assert.h>

#include "thunk_size.h"
#include "type.h"

class ThunkFunc;
class ThunkSize;

typedef std::map<std::string, ThunkFunc*>		thunkfunc_map;

class ThunkType
{
public:
	ThunkType(const Type* t) 
	: t_type(t),
	  t_size(NULL)
	{ assert (t_type != NULL); }

	virtual ~ThunkType(void);
	
	const Type* getType(void) const
	{
		return t_type;
	}

	/* return expression list needed to invoke any field 
	 * or type function on this type 
	 * (at minimum will have __thunk_arg_offset in the first field)
	 */
	ExprList* copyExprList(void) const;

	bool registerFunc(const ThunkFunc* f);

	void setSize(ThunkSize* in_t_size)
	{
		assert (t_size == NULL);
		t_size = in_t_size;
		registerFunc(t_size);
	}

	const ThunkSize* getSize(void) const 
	{
		assert (t_size != NULL);
		return t_size; 
	}

	ThunkType* copy(void) const;

	thunkfunc_map::const_iterator begin(void) const 
	{
		return thunkfuncs_map.begin(); 
	}

	thunkfunc_map::const_iterator end(void) const
	{
		return thunkfuncs_map.end();
	}

	unsigned int getThunkArgCount(void) const
	{
		return t_type->getNumArgs() + 1;
	}

	/* generate code for all registered functions */
	bool genCode(void) const;

	/* generate protos for all registered functions */
	bool genProtos(void) const;

private:
	const Type	*t_type;
	ThunkSize	*t_size;
	thunkfunc_map	thunkfuncs_map;
};
#endif
