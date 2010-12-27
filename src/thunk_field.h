#ifndef THUNKFIELD_H
#define THUNKFIELD_H

#include "thunk_elems.h"
#include "thunk_fieldoffset.h"
#include "thunk_fieldsize.h"
#include "thunk_params.h"

#define TF_FIELDNUM_NONE	((unsigned int)(~0))

/**
 * all thunks take parameters given by parent thunk type
 */
class ThunkField
{
public:
	ThunkField(
		ThunkType&		owner,
		const std::string	&in_fieldname,
		ThunkFieldOffset*	in_off,
		ThunkFieldSize*		in_size,
		ThunkElements*		in_elems,
		ThunkParams*		in_params);

	ThunkField(
		ThunkType&		owner,
		const std::string	&in_fieldname,
		ThunkFieldOffset*	in_off,
		ThunkFieldSize*		in_size,
		ThunkElements*		in_elems,
		ThunkParams*		in_params,
		unsigned int		in_fieldnum);
	virtual ~ThunkField();

	static ThunkField* createInvisible(
		ThunkType		&owner,
		const std::string	&in_fieldname,
		ThunkFieldOffset*	in_off,
		ThunkFieldSize*		in_size);

	const ThunkFieldOffset* getOffset(void) const { return t_fieldoff; }
	const ThunkFieldSize* getSize(void) const { return t_size; }
	const ThunkElements* getElems(void) const { return t_elems; }
	const ThunkParams* getParams(void) const { return t_params; }

	ThunkField* copy(ThunkType& owner) const;

	Expr* copyNextOffset(void) const;
	Expr* copyNumBits(void) const;

	/* get the type of the field */
	const Type* getType(void) const { return t_size->getType(); }

	bool genProtos(void) const;
	bool genCode(void) const;

	const std::string& getFieldName(void) const { return fieldname; }
	unsigned int getFieldNum(void) const { return field_num; }
	const Type* getOwnerType(void) const { return owner_type; }

private:
	void setFields(ThunkType& owner);

	std::string		fieldname;
	ThunkFieldOffset	*t_fieldoff;
	ThunkFieldSize		*t_size;
	ThunkElements		*t_elems;
	ThunkParams		*t_params;

	unsigned int		field_num;

	const Type		*owner_type;
};

#endif
