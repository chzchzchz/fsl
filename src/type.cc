#include "phys_type.h"
#include "type.h"

using namespace std;

static PhysicalType* resolve_by_id(const type_map& tm, Id* id);

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



#if 0
vector<TypeBB> TypeBlock::getBB() const
{
	vector<TypeBB>	ret;
	TypeBB			cur_bb;

	for (const_iterator it = begin(); it != end(); it++) {
		TypeStmt*	s;

		if (dynamic_cast<TypeDecl*>(s) != NULL) {
			cur_bb.add(s);
		} else {
			ret.push_back(cur_bb);
			cur_bb.clear();
		}
	}

	if (cur_bb.size() != 0) {
		ret.push_back(cur_bb);
	}

	return ret;
}
#endif

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

PhysicalType* TypeParamDecl::resolve(const type_map& tm) const
{
	/* XXX STUB */
	cerr << "XXX: resolving param decl" << endl;
	/* this should probably be some sort of thunk type */
	return new PhysTypeEmpty();
}



PhysicalType* TypeDecl::resolve(const type_map& tm) const
{
	if (name != NULL) {
		return resolve_by_id(tm, type);
	} else if (array != NULL) {
		PhysicalType	*base;

		base = resolve_by_id(tm, type);
		if (base == NULL) return NULL;

		return new PhysTypeArray(base, (array->getIdx())->copy());
	} else {
		/* static decl must map to either an array or an id */
		assert (0 == 1);
	}

	return NULL;
}

static PhysicalType* resolve_by_id(const type_map& tm, Id* id)
{
	type_map::const_iterator	it;

	it = tm.find(id->getName());
	if (it == tm.end()) {
		cerr << "Could not resolve type: ";
		cerr << id->getName() << endl;
		return NULL;
	}

	return ((*it).second)->copy();
}

PhysicalType* TypeCond::resolve(const type_map& tm) const
{
	/* XXX */
	cerr << "XXX: resolving cond." << endl;
	return is_true->resolve(tm);
}

PhysicalType* TypeBlock::resolve(const type_map& tm) const
{
	const_iterator		it;
	PhysTypeAggregate	*ret;

	ret = new PhysTypeAggregate("__block");
	for (it = begin(); it != end(); it++) {
		ret->add((*it)->resolve(tm));
	}

	return ret;
}
