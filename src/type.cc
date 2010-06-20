#include "phys_type.h"
#include "type.h"
#include "symtab.h"

using namespace std;

static PhysicalType* resolve_by_id(const ptype_map& tm, const Id* id);

static bool load_sym_cond(
	const TypeCond*		tc,
	const Expr*		base_expr,
	const ptype_map&		tm,
	SymbolTable*		symtab);
static bool load_sym_stmt(
	const TypeStmt*		stmt,
	const Expr*		base_expr, 
	const ptype_map&		tm,
	SymbolTable*		symtab);
static bool load_sym_block(
	const TypeBlock*	tb, 
	const Expr*	 	base_expr,
	const ptype_map&		tm,
	SymbolTable*		symtab);

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

PhysicalType* TypeParamDecl::resolve(const ptype_map& tm) const
{
	/* directly resolving this is impossible, that's why we set the phys
	 * type as a thunk for later resolution */
	ptype_map::const_iterator	it;
	PhysicalType			*pt;
	PhysTypeThunk			*thunk;
	ExprList			*arg_copy;
	const string&			type_name(type->getName());

	it = tm.find(string("thunk_") + type_name);
	if (it == tm.end()) {
		cerr	<< "Could not resolve thunk: \"" << type_name << "\""
			<< endl;
		return NULL;
	}

	pt = ((*it).second)->copy();
	thunk = dynamic_cast<PhysTypeThunk*>(pt);
	assert (thunk != NULL);

	arg_copy = (type->getExprs())->copy();
	if (thunk->setArgs(arg_copy) == false) {
		cerr << "Thunk arg length mismatch on " 
		     << thunk->getType()->getName() << endl;
		delete arg_copy;
		delete thunk;
		return NULL;
	}

	/* if it's an array, we need to wrap in an array type */
	if (array != NULL) {
		pt = new PhysTypeArray(pt, (array->getIdx())->copy());
	}

	return pt;
}

PhysicalType* TypeDecl::resolve(const ptype_map& tm) const
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

static PhysicalType* resolve_by_id(const ptype_map& tm, const Id* id)
{
	const std::string&		id_name(id->getName());
	ptype_map::const_iterator	it;

	/* try to resovle type directly */
	it = tm.find(id_name);
	if (it != tm.end()) {
		return ((*it).second)->copy();
	}

	/* type does not resolve directly, may have a thunk available */
	it = tm.find(string("thunk_") + id_name);
	if (it == tm.end()) {
		cerr << "Could not resolve type: ";
		cerr << id_name << endl;
		return NULL;
	}	

	return ((*it).second)->copy();
}

PhysicalType* TypeFunc::resolve(const ptype_map& tm) const
{
	return new PhysTypeFunc(fcall->copy());
}

PhysicalType* TypeCond::resolve(const ptype_map& tm) const
{
	/* XXX */
	cerr << "XXX: resolving cond." << endl;
	return is_true->resolve(tm);
}

PhysicalType* Type::resolve(const ptype_map& tm) const
{
	PhysicalType	*pt_block;

	pt_block = block->resolve(tm);

	return new PhysTypeUser(this, pt_block);
}

PhysicalType* TypeUnion::resolve(const ptype_map& tm) const
{
	TypeBlock::const_iterator		it;
	PhysTypeUnion				*ret;

	ret = new PhysTypeUnion("__union");
	for (it = block->begin(); it != block->end(); it++) {
		PhysicalType	*resolved;

		resolved = (*it)->resolve(tm);
		if (resolved == NULL) {
			delete ret;
			return NULL;
		}

		ret->add(resolved);
	}

	return ret;
}

PhysicalType* TypeBlock::resolve(const ptype_map& tm) const
{
	const_iterator		it;
	PhysTypeAggregate	*ret;

	ret = new PhysTypeAggregate("__block");
	for (it = begin(); it != end(); it++) {
		PhysicalType	*resolved;

		resolved = (*it)->resolve(tm);
		if (resolved == NULL) {
			delete ret;
			return NULL;
		}

		ret->add(resolved);
	}

	return ret;
}

static bool load_sym_stmt(
	const TypeStmt*		stmt,
	const Expr*		base_expr, 
	const ptype_map&		tm,
	SymbolTable*		symtab)
{
	/* special cases that don't map into symtable  */
	const TypeBlock		*t_block;
	const TypeCond		*t_cond;
	/* cases that do map into the symtable */
	const TypeDecl		*t_decl;
	const TypeParamDecl	*t_pdecl;
	bool			rc;

	t_block = dynamic_cast<const TypeBlock*>(stmt);
	t_cond = dynamic_cast<const TypeCond*>(stmt);
	t_decl = dynamic_cast<const TypeDecl*>(stmt);
	t_pdecl = dynamic_cast<const TypeParamDecl*>(stmt);
	
	rc = true;
	if (t_block != NULL) {
		rc = load_sym_block(t_block, base_expr, tm, symtab);
	} else if (t_cond != NULL) {
		rc = load_sym_cond(t_cond, base_expr, tm, symtab);
	} else if (t_decl != NULL) {
		rc = symtab->add(
			t_decl->getName(),
			stmt->resolve(tm),
			base_expr->copy());
		if (rc == false) {
			cerr	<< "Duplicate symbol: " 
				<< t_decl->getName() << endl;
		}
	} else if (t_pdecl != NULL) {
		rc = symtab->add(
			t_pdecl->getName(),
			stmt->resolve(tm),
			base_expr->copy());
		if (rc == false) {
			cerr	<< "Duplicate symbol: " 
				<< t_pdecl->getName() << endl;
		}
	}

	return rc;
}

static bool load_sym_cond(
	const TypeCond*		tc,
	const Expr*		base_expr,
	const ptype_map&		tm,
	SymbolTable*		symtab)
{
	const TypeStmt*	stmt_false;
	bool		rc;

	rc = load_sym_stmt(tc->getTrueStmt(), base_expr, tm, symtab);
	if (rc == false)
		return rc;

	stmt_false = tc->getFalseStmt();
	if (stmt_false != NULL)
		rc = load_sym_stmt(stmt_false, base_expr, tm, symtab);

	return rc;
}

/* HACK HACK HACK */
static bool load_sym_block(
	const TypeBlock*	tb, 
	const Expr*	 	base_expr,
	const ptype_map&		tm,
	SymbolTable*		symtab)
{
	TypeBlock::const_iterator	it;
	Expr*				cur_expr;
	bool				rc;

	rc = true;
	cur_expr = base_expr->copy();

	for (it = tb->begin(); it != tb->end(); it++) {
		TypeStmt	*cur_stmt;
		PhysicalType	*pt;

		cur_stmt = *it;

		rc = load_sym_stmt(cur_stmt, cur_expr, tm, symtab);
		if (rc == false)
			break;


		pt = cur_stmt->resolve(tm);
		if (pt == NULL) {
			cerr << "Could not resolve type" << endl;
			rc = false;
			break;
		}

		cur_expr = new AOPAdd(
			cur_expr, 
			pt->getBits());

		delete pt;
	}

	delete cur_expr;

	return rc;
}

bool load_sym_args(
	SymbolTable	*sm,
	const ptype_map	&tm,
	const ArgsList	*args)
{
	if (args == NULL) return true;

	for (unsigned int i = 0; i < args->size(); i++) {
		pair<Id*, Id*>	p;
		PhysicalType	*pt;
		ExprList	*exprs;
		bool		rc;

		p = args->get(i);
		pt = resolve_by_id(tm, p.first);
		if (pt == NULL) return false;

		exprs = new ExprList();
		exprs->add(new Number(i));

		rc = sm->add(
			(p.second)->getName(),
			pt,
			new FCall(new Id("__arg"), exprs));

		if (rc == false) return false;
	}

	return true;
}

SymbolTable* Type::getSyms(const ptype_map& tm) const
{
	SymbolTable	*symtab;
	Expr		*cur_expr;
	bool		success;
	
	cur_expr = new Number(0);	/* start it off */
	symtab = new SymbolTable(NULL, resolve(tm));

	/* load symbols defined in argument list */
	success = load_sym_args(symtab, tm, args);
	if (success == false) goto err;

	/* load symbols defined in type block */
	success = load_sym_block(block, cur_expr, tm, symtab);
	if (success == false) goto err;

	delete cur_expr;

	return symtab;
err:
	delete cur_expr;
	delete symtab;
	return NULL;
}

