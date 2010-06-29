#include "phys_type.h"
#include "type.h"
#include "symtab.h"
#include "cond.h"

using namespace std;

static FCall	from_base_fc(new Id("from_base"), new ExprList());

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

PhysicalType* TypeParamDecl::resolve(const Expr* off, const ptype_map& tm) const
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

PhysicalType* TypeDecl::resolve(const Expr* off, const ptype_map& tm) const
{
	if (name != NULL) {
		return resolve_by_id(tm, type);
	} else if (array != NULL) {
		PhysicalType	*base;
		Expr		*e, *e_tmp;

		base = resolve_by_id(tm, type);
		if (base == NULL) return NULL;


		e = (array->getIdx())->simplify();
		e_tmp = e->rewrite(&from_base_fc, off);
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

static PhysicalType* genAssertEq(const ExprList* args, unsigned int lineno)
{
	Expr	*lhs, *rhs;
	Number	*n_lhs, *n_rhs;
	bool	static_checked;

	static_checked = false;

	if (args->size() != 2) {
		cerr << "ASSERT_EQ: Expected two arguments." << endl;
		return NULL;
	}

	lhs = *(args->begin());
	rhs = *(++(args->begin()));

	n_rhs = dynamic_cast<Number*>(rhs);
	n_lhs = dynamic_cast<Number*>(lhs);
	if (n_rhs != NULL && n_lhs != NULL) {
		if (n_rhs->getValue() != n_lhs->getValue()) {
			cerr << lineno << ": ASSERT_EQ failed. ";
			cerr << n_lhs->getValue() << " != " << 
				n_rhs->getValue() << endl;
			return NULL;
		} else {
			static_checked = true;
		}
	}

	if (static_checked) {
		return new PhysTypeEmpty();
	} else {
		cout << "GENERATING FCALL" << endl;
		return new PhysTypeFunc(
			new FCall(
				new Id("assert_eq"), 
				args->copy()));
	}
}

PhysicalType* TypeFunc::resolve(const Expr* off, const ptype_map& tm) const
{
	PhysicalType	*ret;
	string		func_name;
	ExprList	*args, *args_tmp;

	func_name = fcall->getName();

	/* rewrite arguments */
	args = (fcall->getExprs())->simplify();
	args->rewrite(&from_base_fc, off);
	args_tmp = args->simplify();
	delete args;
	args = args_tmp;

	if (func_name == "align") {
		if (args->size() != 1) {
			cerr 	<< getLineNo() << 
				": align takes exactly one argument" << endl;
			ret = NULL;
		} else {
			/* have a proper offset in the binding, align it.. */
			/* note: aligning is (align_v - offset) % align_v */
			ret = new PhysTypeArray(
				new U1(),
				new AOPMod(
					new AOPSub(
						new AOPMul(
						(args->front())->simplify(),
						new Number(8)),
						off->simplify()),
					new AOPMul(
						(args->front())->simplify(),
						new Number(8)))
				);
		}
	} else if (func_name == "skip") {
		if (args->size() != 1) {
			cerr << "skip takes exactly one argument" << endl;
			ret = NULL;
		} else {
			ret = new PhysTypeArray(
				new U8(), (args->front())->simplify());
		}
	} else if(func_name == "assert_eq") {
		ret = genAssertEq(args, getLineNo());
	} else {
		/* don't know what to do with it! */
		ret = new PhysTypeFunc(
			new FCall(
				new Id(fcall->getName()), 
				args->copy()));
	}

done:
	delete args;
	return ret;
}


/* build up cond phys type by drilling down 
 * compound condition expressions into single condition expressions.. */
PhysicalType* TypeCond::resolve(const Expr* off, const ptype_map& tm) const
{
	PhysicalType	*t, *f;

	t = is_true->resolve(off, tm);
	f = (is_false == NULL) ? NULL : is_false->resolve(off, tm);

	return cond_resolve(cond, tm, t, f);
}

PhysicalType* Type::resolve(const Expr* off, const ptype_map& tm) const
{
	PhysicalType	*pt_block;

	pt_block = block->resolve(off, tm);

	return new PhysTypeUser(this, pt_block);
}

PhysicalType* TypeUnion::resolve(const Expr* off, const ptype_map& tm) const
{
	TypeBlock::const_iterator		it;
	PhysTypeUnion				*ret;

	ret = new PhysTypeUnion("__union");
	for (it = block->begin(); it != block->end(); it++) {
		PhysicalType	*resolved;

		resolved = (*it)->resolve(off, tm);
		if (resolved == NULL) {
			delete ret;
			return NULL;
		}

		ret->add(resolved);
	}

	return ret;
}

PhysicalType* TypeBlock::resolve(const Expr* off, const ptype_map& tm) const
{
	const_iterator		it;
	PhysTypeAggregate	*ret;
	Expr			*cur_off;

	ret = new PhysTypeAggregate("__block");
	cur_off = off->simplify();

	for (it = begin(); it != end(); it++) {
		PhysicalType	*resolved;
		Expr		*next_off;

		resolved = (*it)->resolve(cur_off, tm);
		if (resolved == NULL) {
			delete ret;
			break;
		}

		ret->add(resolved);

		cur_off = new AOPAdd(
				cur_off,
				resolved->getBits());
		next_off = cur_off->simplify();
		delete cur_off;
		cur_off = next_off;
	}

	delete cur_off;
	return ret;
}

SymbolTable* Type::getSyms(const ptype_map& tm) const
{
	SymbolTable	*symtab;
	Expr		*cur_expr;
	bool		success;
	
	cur_expr = new Number(0);	/* start it off */
	symtab = new SymbolTable(NULL, resolve(cur_expr, tm));

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
