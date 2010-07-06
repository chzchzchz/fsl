#include "phys_type.h"
#include "type.h"
#include "symtab.h"

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

SymbolTable* Type::getSyms(const ptype_map& tm) const
{
	SymTabBuilder	*sbuilder;
	SymbolTable	*st;
	PhysTypeUser	*ptu;

	sbuilder = new SymTabBuilder(tm);
	st = sbuilder->getSymTab(this, ptu);
	st->setOwner(ptu);
	delete sbuilder;

	return st;
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
