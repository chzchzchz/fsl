#include "phys_type.h"
#include "type.h"
#include "symtab.h"
#include "symtab_builder.h"
#include "symtab_thunkbuilder.h"

using namespace std;

void Type::print(ostream& out) const 
{
	out << "Type (";
	name->print(out);
	out << ") "; 

	out << "Conds=" << block->getNumConds() << endl;

	block->print(out);
	out << endl;
	out << "End Type";
}


Type::~Type(void)
{
	delete name;
	if (args != NULL) delete args;
	if (preamble != NULL) delete preamble;
	if (cached_symtab != NULL) delete cached_symtab;
	if (cached_symtab_thunked != NULL) delete cached_symtab_thunked;
	delete block;
}

TypeDecl::~TypeDecl(void)
{
	delete type;
	if (name != NULL) delete name;
	if (array != NULL) delete array;
}

void TypeDecl::print(ostream& out) const
{
	out << "DECL: "; 
	if (array != NULL)	array->print(out);
	else			name->print(out);
	out << " -> ";
	type->print(out);
}

void TypeBlock::print(ostream& out) const
{
	out << "BLOCK:" << endl; 
	for (const_iterator it = begin(); it != end(); it++) {
		(*it)->print(out);
		out << endl;
	}
	out << "ENDBLOCK";
}

void TypeParamDecl::print(std::ostream& out) const 
{
	out << "PDECL: "; 
	if (array != NULL)	array->print(out);
	else			name->print(out);
	out << " -> ";
	type->print(out);
}

void Type::buildSyms(const ptype_map& tm)
{
	PhysTypeUser	*ptu;
	SymTabBuilder	*sbuilder;

	assert (cached_symtab == NULL);

	sbuilder = new SymTabBuilder(tm);
	cached_symtab = sbuilder->getSymTab(this, ptu);
	cached_symtab->setOwner(ptu);
	delete sbuilder;
}

void Type::buildSymsThunked(const ptype_map& tm)
{
	SymTabThunkBuilder	*sbuilder;
	PhysicalType		*ptt;

	assert (cached_symtab_thunked == NULL);

	sbuilder = new SymTabThunkBuilder(tm);
	cached_symtab_thunked = sbuilder->getSymTab(this, ptt);
	cached_symtab_thunked->setOwner(ptt);
	delete sbuilder;
}

SymbolTable* Type::getSyms(const ptype_map& tm) const
{
	assert (cached_symtab != NULL);
	return cached_symtab->copy();
}

SymbolTable* Type::getSymsThunked(const ptype_map& tm) const
{
	assert (cached_symtab_thunked != NULL);
	return cached_symtab_thunked->copy();
}

SymbolTable* Type::getSymsByUserType(const ptype_map& tm) const
{
	SymbolTable	*ret;

	assert (cached_symtab != NULL);

	ret = new SymbolTable(NULL, cached_symtab->getOwner()->copy());
	for (	sym_map::const_iterator it = cached_symtab->begin();
		it != cached_symtab->end();
		it++) 
	{
		const std::string	name = (*it).first;
		const SymbolTableEnt*	st_ent = (*it).second;

		if (isPTaUserType(st_ent->getPhysType()) == false)
			continue;

		ret->add(name, st_ent);
			
	}

	return ret;
}



/* find all preamble entries that match given name */
list<const FCall*> Type::getPreambles(const std::string& name) const
{
	list<const FCall*>		ret;
	TypePreamble::const_iterator	it;

	if (preamble == NULL)
		return ret;

	if (preamble->size() == 0)
		return ret;

	for (it = preamble->begin(); it != preamble->end(); it++) {
		FCall	*fc = *it;
		if (fc->getName() == name)
			ret.push_back(fc);
	}

	return ret;
}

std::ostream& operator<<(std::ostream& in, const Type& t)
{
	t.print(in);
	return in;
}


PhysicalType* Type::resolve(const ptype_map& tm) const
{
	SymTabBuilder	*sbuilder;
	SymbolTable	*st;
	PhysTypeUser	*ptu;

	sbuilder = new SymTabBuilder(tm);
	st = sbuilder->getSymTab(this, ptu);
	delete sbuilder;
	delete st;

	return ptu;
}


const Type* ptype_map::getUserType(const std::string& name) const
{
	const PhysicalType	*pt;

	pt = getPT(name);
	if (pt == NULL)
		return NULL;


	return PT2Type(pt);
}

const PhysicalType* ptype_map::getPT(const std::string& name) const
{
	const PhysicalType		*pt;
	const_iterator			it;

	/* try to resolve type directly */
	it = find(name);
	if (it != end()) {
		return (*it).second;
	}

	/* type does not resolve directly, may have a thunk available */
	it = find(std::string("thunk_") + name);
	if (it == end()) {
		return NULL;
	}	

	return ((*it).second);
}
