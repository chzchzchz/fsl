#include <iostream>

#include "symtab.h"
#include "type.h"
#include "expr.h"

using namespace std;

extern const_map		constants;
extern symtab_map		symtabs_thunked;

FCall	from_base_fc(new Id("from_base"), new ExprList());

bool SymbolTable::loadArgs(const ptype_map& tm, const ArgsList *args)
{
	if (args == NULL) return true;

	for (unsigned int i = 0; i < args->size(); i++) {
		pair<Id*, Id*>  p;
		PhysicalType    *pt;
		bool            rc;

		p = args->get(i);
		pt = resolve_by_id(tm, p.first);
		if (pt == NULL) return false;


//		rc = add(
//			(p.second)->getName(),
//			pt,
//			new FCall(new Id("__arg"), exprs));
//
		rc = add((p.second)->getName(), pt, (p.second)->copy());


		if (rc == false) return false;
	}

	return true;
}

void SymbolTable::print(ostream& out) const
{
	sym_map::const_iterator	it;

	out << "SymTable for " << owner->getName() << std::endl;
	for (it = sm.begin(); it != sm.end(); it++) {
		const SymbolTableEnt	*st_ent = (*it).second;

		out 	<< "Symbol: \"" <<  (*it).first << "\" = ";
		out << (st_ent->getPhysType())->getName();
		out << '@';
		st_ent->getOffset()->print(out);
		out << std::endl;
	}
}

SymbolTable* SymbolTable::copy(void)
{
	SymbolTable	*new_st;

	new_st = new SymbolTable(
		backref, 
		(owner) ? owner->copy() : NULL);

	new_st->copyInto(*this);

	return new_st;
}

SymbolTable& SymbolTable::operator=(const SymbolTable& st)
{
	copyInto(st);
	return *this;
}

void SymbolTable::copyInto(const SymbolTable& st, Expr* new_base)
{
	sym_map::const_iterator	it;

	if (&st == this)
		return;

	freeData();

	owner = (st.owner)->copy();
	backref = st.backref;

	for (it = st.sm.begin(); it != st.sm.end(); it++) {
		const SymbolTableEnt	*st_ent;

		st_ent = (*it).second;
		if (new_base == NULL) {
			add(	(*it).first,
				st_ent->getPhysType()->copy(),
				st_ent->getOffset()->copy());
		} else {
			add(	(*it).first,
				st_ent->getPhysType()->copy(),
				new AOPAdd(
				new_base->copy(), 
				st_ent->getOffset()->copy()));
		}
	}
}

const SymbolTableEnt* SymbolTable::lookup(const std::string& name) const
{
	sym_map::const_iterator	it;
	
	if (sm.count(name) == 0) {
		if (backref == NULL)
			return false;

		return backref->lookup(name);
	}

	it = sm.find(name);
	if (it == sm.end()) return false;

	return ((*it).second);
}

bool SymbolTable::add(const std::string& name, const SymbolTableEnt* st_ent)
{
	return add(
		name, 
		st_ent->getPhysType()->copy(),
		st_ent->getOffset()->copy(),
		st_ent->isWeak());
}

bool SymbolTable::add(
	const std::string& name, PhysicalType* pt, Expr* offset_bits,
	bool weak_binding)
{
	if (sm.count(name) != 0) {
		/* exists */
		return false;
	}

	sm[name] = new SymbolTableEnt(
		"__XXX_ANTHONY_NEEDS_TYPENAME", 
		name,
		pt,
		offset_bits,
		weak_binding);
	return true;
}

const Type* SymbolTable::getOwnerType(void) const
{ 
	const PhysTypeUser	*ptu;
	const PhysTypeThunk	*ptt;

	ptu = dynamic_cast<const PhysTypeUser*>(getOwner());
	if (ptu != NULL) {
		return ptu->getType();
	}

	ptt = dynamic_cast<const PhysTypeThunk*>(getOwner());
	if (ptt != NULL ) {
		return ptt->getType();
	}

	return NULL;
}

void SymbolTable::setOwner(PhysicalType* new_owner)
{
	assert (owner == NULL);
	owner = new_owner;
}


void SymbolTable::freeData(void)
{
	sym_map::iterator	it;

	for (it = sm.begin(); it != sm.end(); it++) {
		SymbolTableEnt	*st_ent = ((*it).second);
		delete st_ent;
	}

	sm.clear();

	if (owner != NULL) {
		delete owner;
		owner = NULL;
	}
}
