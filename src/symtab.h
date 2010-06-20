#ifndef SYMTAB_H
#define SYMTAB_H

#include <iostream>
#include <map>
#include "phys_type.h"

typedef std::pair<PhysicalType*, Expr*>		sym_binding;
typedef std::map<std::string, sym_binding>	sym_map;

#define symbind_make(x,y)	(sym_binding(x,y))
#define symbind_phys(x)		((x).first)
#define symbind_off(x)		((x).second)

class SymbolTable
{
public:
	SymbolTable(
		const SymbolTable*	in_backref, 
		PhysicalType*		in_owner)
	: backref(in_backref),
	  owner(in_owner)
	{
		assert (owner != NULL);
	}

	virtual ~SymbolTable()
	{
		sym_map::iterator	it;

		for (it = sm.begin(); it != sm.end(); it++) {
			sym_binding	sb((*it).second);

			delete symbind_phys(sb);
			delete symbind_off(sb);
		}

		delete owner;
	}

	bool add(const std::string& name, PhysicalType* pt, Expr* offset_bits)
	{
		if (sm.count(name) != 0) {
			/* exists */
			return false;
		}

		sm[name] = symbind_make(pt,offset_bits);
		return true;
	}

	bool lookup(const std::string& name, sym_binding& sb) const
	{
		sym_map::const_iterator	it;

		if (sm.count(name) == 0) {
			if (backref == NULL)
				return false;

			return backref->lookup(name, sb);
		}

		it = sm.find(name);
		if (it == sm.end()) return false;

		sb = ((*it).second);
		return true;
	}

	void print(std::ostream& out) const
	{
		sym_map::const_iterator	it;

		out << "SymTable for " << owner->getName() << std::endl;
		for (it = sm.begin(); it != sm.end(); it++) {
			sym_binding	sb((*it).second);
			out 	<< "Symbol: \"" <<  (*it).first << "\" = ";
			out << symbind_phys(sb)->getName();
			out << '@';
			symbind_off(sb)->print(out);
			out << std::endl;
		}
	}

private:
	const SymbolTable*	backref;
	PhysicalType*		owner;
	sym_map			sm;

};

#endif
