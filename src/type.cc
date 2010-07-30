#include "phys_type.h"
#include "type.h"
#include "symtab.h"
#include "symtab_thunkbuilder.h"

using namespace std;

extern symtab_map	symtabs;

void Type::print(ostream& out) const 
{
	out << "Type (";
	name->print(out);
	out << ") "; 

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

void Type::buildSyms(void)
{
	SymTabThunkBuilder	*sbuilder;
	list<SymbolTable*>	union_symtabs;

	assert (cached_symtab == NULL);

	sbuilder = new SymTabThunkBuilder();
	cached_symtab = sbuilder->getSymTab(this, union_symtabs);
	delete sbuilder;

	/* TODO: make this cleaner.. no globals!! */
	for (	list<SymbolTable*>::const_iterator it = union_symtabs.begin();
		it != union_symtabs.end();
		it++)
	{
		SymbolTable	*st = *it;
		symtabs[st->getOwnerType()->getName()] = st;
	}
}

SymbolTable* Type::getSyms(void) const
{
	assert (cached_symtab != NULL);
	return cached_symtab->copy();
}

SymbolTable* Type::getSymsByUserType() const
{
	SymbolTable	*ret;

	assert (cached_symtab != NULL);

	ret = new SymbolTable(cached_symtab->getThunkType()->copy());
	for (	sym_map::const_iterator it = cached_symtab->begin();
		it != cached_symtab->end();
		it++) 
	{
		const std::string	name = (*it).first;
		const SymbolTableEnt*	st_ent = (*it).second;

		if (st_ent->getFieldThunk()->getType() == NULL)
			continue;

		ret->add(st_ent);
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

