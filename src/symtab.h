#ifndef SYMTAB_H
#define SYMTAB_H

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
		const SymbolTable* in_backref, 
		const PhysicalType* in_owner)
	: backref(in_backref),
	  owner(in_owner)
	{

	}

	virtual ~SymbolTable()
	{
		sym_map::iterator	it;

		for (it = sm.begin(); it != sm.end(); it++) {
			sym_binding	sb(*it);

			delete symbind_phys(sb);
			delete symbind_off(sb);
		}
	}

	bool add(const std::string& name, PhysicalType* pt, Expr* offset)
	{
		if (sm.count(name) != 0) {
			/* exists */
			return false;
		}

		sm[name] = symbind_make(pt,offset);
		return true;
	}

	bool lookup(const std::string& name, sym_binding& sb) const
	{
		if (sm.count(name) == 0) {
			return false;
		}

		sb = sm[name];
		return true;
	}

private:
	const SymbolTable*	backref;
	const PhysicalType*	owner;
	sym_map			sm;

};

#endif
