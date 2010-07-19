#ifndef SYMTAB_H
#define SYMTAB_H

#include <iostream>
#include <map>
#include "phys_type.h"

typedef std::map<std::string, class SymbolTableEnt*>		sym_map;
typedef std::map<std::string, SymbolTable*>		symtab_map;

class SymbolTableEnt
{
public:
	SymbolTableEnt(
		const std::string	&in_typename,
		const std::string	&in_fieldname,
		PhysicalType		*in_pt,
		class Expr		*in_offset,
		bool			in_is_weak) 
	: type_name(in_typename),
	fieldname(in_fieldname),
	pt(in_pt),
	offset(in_offset),
	is_weak(in_is_weak)
	{
		assert (pt != NULL);
		assert (offset != NULL);
	}

	const Expr* getOffset(void) const { return offset; }
	const PhysicalType* getPhysType(void) const { return pt; }
	const std::string& getTypeName(void) const { return type_name; }
	const std::string& getFieldName(void) const { return fieldname; }
	bool isWeak(void) const { return is_weak; }

	virtual ~SymbolTableEnt(void) 
	{ 
		delete pt;
		delete offset;
	}

private:
	std::string		type_name;
	std::string		fieldname;
	PhysicalType		*pt;
	Expr			*offset;
	bool			is_weak;
};

/**
 * TODO: GENERATE TABLE THAT CAN BE USED AT RUN-TIME TO RESOLVE TYPE 
 * NAMES!
 */
class SymbolTable
{
public:
	SymbolTable(
		const SymbolTable*	in_backref, 
		PhysicalType*		in_owner)
	: backref(in_backref),
	  owner(in_owner) 
	{}

	virtual ~SymbolTable() { freeData(); }

	void setOwner(PhysicalType* new_owner);

	const PhysicalType* getOwner(void) const { return owner; }

	const Type* getOwnerType(void) const;

	bool add(
		const std::string& name, PhysicalType* pt, Expr* offset_bits, 
		bool weak_binding = false);
	bool add(const std::string& name, const SymbolTableEnt* st_ent);
	const SymbolTableEnt* lookup(const std::string& name) const;

	void print(std::ostream& out) const;
	SymbolTable& operator=(const SymbolTable& st);
	SymbolTable* copy(void);
	void copyInto(const SymbolTable& st, Expr* new_base = NULL);

	bool loadArgs(const ptype_map& tm, const ArgsList* args);
	const SymbolTable* getBackRef(void) const { return backref; }

	sym_map::const_iterator	begin(void) const { return sm.begin(); }
	sym_map::const_iterator end(void) const { return sm.end(); }
private:

	void freeData(void);
	const SymbolTable*	backref;
	PhysicalType*		owner;
	sym_map			sm;
};


#endif
