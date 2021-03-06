#include <iostream>

#include "symtab.h"
#include "type.h"
#include "expr.h"

using namespace std;

extern const_map		constants;
extern symtab_map		symtabs;

void SymbolTable::print(ostream& out) const
{
	out << "SymTable for " << owner->getType()->getName() << std::endl;
	for (auto const &p : sm) {
		out << "Symbol: \"" <<  p.first << "\" = ";
		out << p.second->getTypeName();
		out << std::endl;
	}
}

SymbolTable* SymbolTable::copy(void) const
{
	auto new_st = new SymbolTable(owner->copy());
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
	if (&st == this) return;

	freeData();

	owner = (st.owner)->copy();
	for (const auto st_ent : st.sl) {
		add(	st_ent->getFieldName(),
			st_ent->getTypeName(),
			st_ent->getFieldThunk()->copy(*owner),
			st_ent->isWeak());
	}
}

const SymbolTableEnt* SymbolTable::lookup(const std::string& name) const
{
	sym_map::const_iterator	it;

	if (sm.count(name) == 0) return NULL;

	it = sm.find(name);
	if (it == sm.end()) return NULL;

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

	/* exists? */
	if (sm.count(name) != 0) return false;

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

const Type* SymbolTable::getOwnerType(void) const { return owner->getType(); }

void SymbolTable::freeData(void)
{
	for (auto sym : sl) delete sym;
	sm.clear();
	sl.clear();

	delete owner;
	owner = NULL;
}

const ThunkType* SymbolTable::getThunkType(void) const { return owner->copy(); }
bool SymbolTableEnt::isUserType(void) const { return (getType() != NULL); }

const Type* SymbolTableEnt::getType(void) const
{
	return getFieldThunk()->getType();
}

const SymbolTable* symtabByName(const std::string& s)
{
	symtab_map::const_iterator	it;
	it = symtabs.find(s);
	if (it == symtabs.end()) return NULL;
	return (*it).second;
}
