#include "phys_type.h"
#include "type.h"
#include "symtab.h"

using namespace std;

static PhysicalType* cond_cmpop_resolve(
	const CmpOp*		cmpop,
	const ptype_map&	tm,
	PhysicalType		*t,
	PhysicalType		*f);

static PhysicalType* cond_bop_resolve(
	const BinBoolOp*	bop,
	const ptype_map&	tm,
	PhysicalType		*t,
	PhysicalType		*f);

static PhysicalType* cond_resolve(
	const CondExpr* cond, 
	const ptype_map& tm,
	PhysicalType* t, PhysicalType* f);

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

PhysicalType* TypeParamDecl::resolve(const ptype_map& tm) const
{
	/* directly resolving this is impossible, that's why we set the phys
	 * type as a thunk for later resolution */
	ptype_map::const_iterator	it;
	PhysicalType			*pt;
	PhysTypeThunk			*thunk;
	ExprList			*arg_copy;
	const string&			type_name(type->getName());

	/* get the thunk */
	it = tm.find(string("thunk_") + type_name);
	if (it == tm.end()) {
		cerr	<< "Could not resolve thunk: \"" << type_name << "\""
			<< endl;
		return NULL;
	}

	/* into the right type.. */
	pt = ((*it).second)->copy();
	thunk = dynamic_cast<PhysTypeThunk*>(pt);
	assert (thunk != NULL);

	/* set the args */
	/* XXX add support for from_base in args */
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
		/* XXX add support for from_base */
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
		Expr		*e, *e_tmp;
		FCall		*tmp_fc;
		SymbolTable	*st;
		sym_binding	sb;
		bool		found;

		base = resolve_by_id(tm, type);
		if (base == NULL) return NULL;

		st = getOwner()->getSyms(tm);
		assert (st != NULL);
		found = st->lookup(getName(), sb);
		delete st;

		if (found == false) {
			return new PhysTypeArray(
				base, (array->getIdx())->simplify());
		}

		e = (array->getIdx())->simplify();
		tmp_fc = new FCall(new Id("from_base"), new ExprList());
		e_tmp = e->rewrite(tmp_fc, symbind_off(sb));
		if (e_tmp != NULL) {
			delete e;
			e = e_tmp;
		}
		
		return new PhysTypeArray(base, e);
	} else {
		/* static decl must map to either an array or an id */
		assert (0 == 1);
	}
	

	return NULL;
}

PhysicalType* TypeFunc::resolve(const ptype_map& tm) const
{
	string		func_name;
	const ExprList	*args;

	func_name = fcall->getName();
	args = fcall->getExprs();

	if (func_name == "align") {
		SymbolTable	*st;
		sym_binding	sb;
		bool		found;

		if (args->size() != 1) {
			cerr << "align takes exactly one argument" << endl;
			return NULL;
		}

		st = getOwner()->getSyms(tm);
		assert (st != NULL);

		found = st->lookup(getName(), sb);
		delete st;

		if (found == false) {
			/* first round of recursion, return typefunc */
			/* XXX this has potential for an O(n^2) algorithm!
			 * oh fucking well */
			return new PhysTypeFunc(fcall->copy());
		}


		/* have a proper offset in the binding, align it.. */
		/* note: aligning is (align_v - offset) % align_v */
		return new PhysTypeArray(
			new U8(),
			new AOPMod(
				new AOPSub(
					(args->front())->simplify(),
					symbind_off(sb)->simplify()),
				(args->front())->simplify())
			);
	} else if (func_name == "skip") {
		if (args->size() != 1) {
			cerr << "skip takes exactly one argument" << endl;
			return NULL;
		}
		return new PhysTypeArray(new U8(), (args->front())->simplify());
	} else if(func_name == "assert_eq") {
		/* XXX STUB STUB STUB */
		assert (0 == 1);
	}
	
	/* don't know what to do with it! */
	return new PhysTypeFunc(fcall->copy());
}


static PhysicalType* cond_cmpop_resolve(
	const CmpOp*		cmpop,
	const ptype_map&	tm,
	PhysicalType		*t,
	PhysicalType		*f)
{
	Expr		*lhs, *rhs;

	assert (cmpop != NULL);

	lhs = (cmpop->getLHS())->simplify();
	rhs = (cmpop->getRHS())->simplify();

	switch (cmpop->getOp()) {
	case CmpOp::EQ: return new PhysTypeCondEQ(lhs, rhs, t, f);
	case CmpOp::NE: return new PhysTypeCondNE(lhs, rhs, t, f);
	case CmpOp::LE: return new PhysTypeCondLE(lhs, rhs, t, f);
	case CmpOp::LT: return new PhysTypeCondLT(lhs, rhs, t, f);
	case CmpOp::GT: return new PhysTypeCondGT(lhs, rhs, t, f);
	case CmpOp::GE: return new PhysTypeCondGE(lhs, rhs, t, f);
	default:
		assert (0 == 1);
	}

	return NULL;
}

static PhysicalType* cond_bop_resolve(
	const BinBoolOp*	bop,
	const ptype_map& 	tm,
	PhysicalType*		t,
	PhysicalType*		f)
{
	const CondExpr*	cond_lhs;
	const CondExpr*	cond_rhs;

	cond_lhs = bop->getLHS();
	cond_rhs = bop->getRHS();

	assert (bop != NULL);

	if ((dynamic_cast<const BOPAnd*>(bop)) != NULL) {
		/* if LHS evaluates to true, do RHS */

		return cond_resolve(
			cond_lhs,
			tm,
			cond_resolve(cond_rhs, tm, t, f), 
			f);
	} else {
		/* must be an OR */
		assert (dynamic_cast<const BOPOr*>(bop) != NULL);

		return cond_resolve(
			cond_lhs,
			tm,
			t,
			cond_resolve(cond_rhs, tm, t, f));
	}

	/* should not happen */
	assert (0 == 1);
	return NULL;
}

static PhysicalType* cond_resolve(
	const CondExpr* cond, 
	const ptype_map& tm,
	PhysicalType* t, PhysicalType* f)
{
	const CmpOp	*cmpop;
	const BinBoolOp	*bop;

	cmpop = dynamic_cast<const CmpOp*>(cond);
	if (cmpop != NULL) {
		return cond_cmpop_resolve(cmpop, tm, t, f);
	}

	bop = dynamic_cast<const BinBoolOp*>(cond);
	assert (bop != NULL);

	return cond_bop_resolve(bop, tm, t, f);

}

/* build up cond phys type by drilling down 
 * compound condition expressions into single condition expressions.. */
PhysicalType* TypeCond::resolve(const ptype_map& tm) const
{
	PhysicalType	*t, *f;

	t = is_true->resolve(tm);
	f = (is_false == NULL) ? NULL : is_false->resolve(tm);

	return cond_resolve(cond, tm, t, f);
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

SymbolTable* Type::getSyms(const ptype_map& tm) const
{
	SymbolTable	*symtab;
	Expr		*cur_expr;
	bool		success;
	
	cur_expr = new Number(0);	/* start it off */
	symtab = new SymbolTable(NULL, resolve(tm));

	/* load symbols defined in argument list */
	success = symtab->loadArgs(tm, args);
	if (success == false) goto err;

	/* load symbols defined in type block */
	success = symtab->loadTypeBlock(tm, cur_expr, block);
	if (success == false) goto err;

	delete cur_expr;

	return symtab;
err:
	delete cur_expr;
	delete symtab;
	return NULL;
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
