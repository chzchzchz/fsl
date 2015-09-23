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

Type::Type(	Id* in_name, ArgsList* in_args, TypePreamble* in_preamble,
		TypeBlock* in_block, bool is_union)
: name(in_name),
  args(in_args),
  preamble(in_preamble),
  block(in_block),
  type_num(-1),
  cached_symtab(NULL),
  is_union_type(is_union)
{
	assert (in_name != NULL);
	assert (in_block != NULL);
	in_block->setOwner(this);
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

void TypeCond::print(std::ostream& out) const
{
	out << "(TYPECOND ";
	is_true->print(out);
	if (is_false != NULL) {
		out << ' ';
		is_false->print(out);
	}
	out << ")";
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
	SymbolTable	*ret;

	assert (cached_symtab != NULL);
	ret = cached_symtab->copy();
	return ret;
}

SymbolTable* Type::getSymsByUserType() const
{
	SymbolTable	*ret;

	assert (cached_symtab != NULL);

	ret = new SymbolTable(cached_symtab->getThunkType()->copy());
	for (	sym_list::const_iterator it = cached_symtab->begin();
		it != cached_symtab->end();
		it++)
	{
		const SymbolTableEnt*	st_ent = *it;

		if (st_ent->getFieldThunk()->getType() == NULL) continue;
		ret->add(st_ent);
	}

	return ret;
}

SymbolTable* Type::getSymsByUserTypeStrong() const
{
	SymbolTable	*ret;

	assert (cached_symtab != NULL);

	ret = new SymbolTable(cached_symtab->getThunkType()->copy());
	for (	sym_list::const_iterator it = cached_symtab->begin();
		it != cached_symtab->end();
		it++)
	{
		const SymbolTableEnt*	st_ent = *it;

		if (st_ent->getFieldThunk()->getType() == NULL) continue;
		if (st_ent->isWeak()) continue;
		ret->add(st_ent);
	}

	return ret;
}

/* get all strong types */
SymbolTable* Type::getSymsStrong(void) const
{
	assert (0 == 1);
	return NULL;
}

/* get all strong types or those that can be verified conditionally */
SymbolTable* Type::getSymsStrongOrConditional(void) const
{
	SymbolTable	*ret;

	assert (cached_symtab != NULL);

	ret = new SymbolTable(cached_symtab->getThunkType()->copy());
	for (	sym_list::const_iterator it = cached_symtab->begin();
		it != cached_symtab->end();
		it++)
	{
		const SymbolTableEnt*	st_ent = *it;
		/* accept everything but the super-weak */
		if (st_ent->isWeak() && !st_ent->isConditional()) continue;
		ret->add(st_ent);
	}

	return ret;
}

SymbolTable* Type::getSymsByUserTypeStrongOrConditional(void) const
{
	SymbolTable	*ret;

	assert (cached_symtab != NULL);

	ret = new SymbolTable(cached_symtab->getThunkType()->copy());
	for (	sym_list::const_iterator it = cached_symtab->begin();
		it != cached_symtab->end();
		it++)
	{
		const SymbolTableEnt*	st_ent = *it;

		/* no super-weaks */
		if (st_ent->isWeak() && !st_ent->isConditional()) continue;
		if (st_ent->isUserType() == false) continue;
		ret->add(st_ent);
	}

	return ret;
}


std::ostream& operator<<(std::ostream& in, const Type& t)
{
	t.print(in);
	return in;
}

list<const Preamble*> TypePreamble::findByName(const string& n) const
{
	list<const Preamble*>	ret;
	for (const auto &p : *this) {
		if (p->getName() == n) ret.push_back(p.get());
	}
	return ret;
}

list<const Preamble*> Type::getPreambles(const string& name) const
{
	if (preamble == NULL) return list<const Preamble*>();
	return preamble->findByName(name);
}


void Type::addPreamble(Preamble* p)
{
	if (preamble == NULL) preamble = new TypePreamble();
	preamble->add(p);
}
