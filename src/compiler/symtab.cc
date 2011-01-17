#include <iostream>

#include "symtab.h"
#include "type.h"
#include "expr.h"

using namespace std;

extern const_map		constants;

void SymbolTable::print(ostream& out) const
{
	sym_map::const_iterator	it;

	out << "SymTable for " << owner->getType()->getName() << std::endl;
	for (it = sm.begin(); it != sm.end(); it++) {
		const SymbolTableEnt	*st_ent = (*it).second;

		out 	<< "Symbol: \"" <<  (*it).first << "\" = ";
		out << st_ent->getTypeName();
		out << std::endl;
	}
}

SymbolTable* SymbolTable::copy(void)
{
	SymbolTable	*new_st;

	new_st = new SymbolTable(owner->copy());
	new_st->copyInto(*this);

	return new_st;
}

SymbolTable& SymbolTable::operator=(const SymbolTable& st)
{
	copyInto(st);
	return *this;
}

void SymbolTable::copyInto(const SymbolTable& st)
{
	sym_list::const_iterator	it;

	if (&st == this)
		return;

	freeData();

	owner = (st.owner)->copy();

	for (it = st.sl.begin(); it != st.sl.end(); it++) {
		const SymbolTableEnt	*st_ent;

		st_ent = (*it);
		add(	st_ent->getFieldName(),
			st_ent->getTypeName(),
			st_ent->getFieldThunk()->copy(*owner),
			st_ent->isWeak());
	}
}

const SymbolTableEnt* SymbolTable::lookup(const std::string& name) const
{
	sym_map::const_iterator	it;

	if (sm.count(name) == 0) {
		return false;
	}

	it = sm.find(name);
	if (it == sm.end()) return false;

	return ((*it).second);
}

bool SymbolTable::add(const SymbolTableEnt* st_ent)
{
	return add(
		st_ent->getFieldName(),
		st_ent->getTypeName(),
		st_ent->getFieldThunk()->copy(*owner),
		st_ent->isWeak(),
		st_ent->isConditional());
}

bool SymbolTable::add(
	const std::string&	name,
	const std::string&	type_name,
	ThunkField*		thunk_field,
	bool			weak_binding,
	bool			cond_binding)
{
	SymbolTableEnt	*st_ent;

	if (sm.count(name) != 0) {
		/* exists */
		return false;
	}

	/* all conditional bindings should be weak */
	assert (!(weak_binding == false && cond_binding));

	st_ent =  new SymbolTableEnt(
		type_name,
		name,
		thunk_field,
		weak_binding,
		cond_binding);

	sm[name] = st_ent;
	sl.push_back(st_ent);

	return true;
}

const Type* SymbolTable::getOwnerType(void) const
{
	return owner->getType();
}

void SymbolTable::freeData(void)
{
	sym_list::iterator	it;

	for (it = sl.begin(); it != sl.end(); it++) {
		delete (*it);
	}

	sm.clear();
	sl.clear();

	if (owner != NULL) {
		delete owner;
		owner = NULL;
	}
}

const ThunkType* SymbolTable::getThunkType(void) const
{
	return owner->copy();
}

bool SymbolTableEnt::isUserType(void) const
{
	return (getType() != NULL);
}


const Type* SymbolTableEnt::getType(void) const
{
	 return getFieldThunk()->getType();
}
