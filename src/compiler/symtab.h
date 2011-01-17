#ifndef SYMTAB_H
#define SYMTAB_H

#include <iostream>
#include <map>
#include "thunk_type.h"
#include "thunk_field.h"

typedef std::map<std::string, class SymbolTableEnt*>	sym_map;
typedef std::list<SymbolTableEnt*>			sym_list;
typedef std::map<std::string, SymbolTable*>		symtab_map;

class SymbolTableEnt
{
public:
	SymbolTableEnt(
		const std::string	&in_typename,
		const std::string	&in_fieldname,
		ThunkField		*in_thunk_field,
		bool			in_is_weak,
		bool			in_is_conditional)
	: type_name(in_typename),
	fieldname(in_fieldname),
	thunk_field(in_thunk_field),
	is_weak(in_is_weak),
	is_conditional(in_is_conditional)
	{
		assert (thunk_field != NULL);
	}

	const Type* getType(void) const;
	const std::string& getTypeName(void) const { return type_name; }
	const std::string& getFieldName(void) const { return fieldname; }
	const ThunkField* getFieldThunk(void) const { return thunk_field; }

	bool isUserType(void) const;
	bool isWeak(void) const { return is_weak; }
	bool isConditional(void) const { return is_conditional; }

	virtual ~SymbolTableEnt(void)
	{
		delete thunk_field;
}

private:
	std::string		type_name;
	std::string		fieldname;
	ThunkField		*thunk_field;
	bool			is_weak;
	bool			is_conditional;

};

class SymbolTable
{
public:
	SymbolTable(ThunkType* in_owner)
	: owner(in_owner)
	{ assert (owner != NULL); }

	virtual ~SymbolTable() { freeData(); }

	const Type* getOwnerType(void) const;
	const ThunkType* getThunkType(void) const;

	bool add(
		const std::string& 	name,
		const std::string&	type_name,
		ThunkField		*thunk_field,
		bool weak_binding = false,
		bool cond_binding = false);
	bool add(const SymbolTableEnt* st_ent);
	const SymbolTableEnt* lookup(const std::string& name) const;

	void print(std::ostream& out) const;
	SymbolTable& operator=(const SymbolTable& st);
	SymbolTable* copy(void);
	void copyInto(const SymbolTable& st);

	sym_list::const_iterator begin(void) const { return sl.begin(); }
	sym_list::const_iterator end(void) const { return sl.end(); }

	unsigned int size(void) const { return sm.size(); }
private:
	void freeData(void);
	ThunkType*		owner;
	sym_map			sm;
	sym_list		sl;
};

inline static void dump_symlist(const SymbolTable& st)
{
	unsigned int	i = 0;
	std::cerr << "dumping symlist " << st.getOwnerType()->getName() << "(" <<
		&st << ") (in order)\n";
	for (	sym_list::const_iterator it = st.begin();
		it != st.end();
		it++, i++)
	{
		const SymbolTableEnt	*st_ent = *it;
		std::cerr << i << ". " << st_ent->getFieldName() << '\n';
	}
	std::cerr << "done.\n";
}

#endif
