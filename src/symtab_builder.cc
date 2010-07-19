#include <iostream>
#include "symtab_builder.h"
#include "cond.h"

using namespace std;

extern const_map		constants;
extern symtab_map		symtabs_thunked;
extern FCall			from_base_fc;


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
		return new PhysTypeFunc(
			new FCall(new Id("assert_eq"), args->copy()));
	}
}

static PhysicalType* genAssertLE(const ExprList* args, unsigned int lineno)
{
	Expr	*lhs, *rhs;
	Number	*n_lhs, *n_rhs;
	bool	static_checked;

	static_checked = false;

	if (args->size() != 2) {
		cerr << "ASSERT_LE: Expected two arguments." << endl;
		return NULL;
	}

	lhs = *(args->begin());
	rhs = *(++(args->begin()));

	n_rhs = dynamic_cast<Number*>(rhs);
	n_lhs = dynamic_cast<Number*>(lhs);
	if (n_rhs != NULL && n_lhs != NULL) {
		if (n_rhs->getValue() >= n_lhs->getValue()) {
			cerr << lineno << ": ASSERT_LE failed. ";
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
		cerr << "XXX NEEDS DYNAMIC ASSERT_LE" << endl;
//		return new PhysTypeFunc(
//			new FCall(new Id("assert_le"), args->copy()));
		return new PhysTypeEmpty();
	}
}





SymbolTable* SymTabBuilder::getSymTab(const Type* t, PhysTypeUser* &ptu)
{
	SymbolTable	*ret;

	assert (cur_symtab == NULL);
	assert (off_thunked == NULL);
	assert (off_pure == NULL);
	assert (ret_pt == NULL);

	union_c = 0;
	off_thunked = new Id("__thunk_off_arg");
	off_pure = new Number(0);

	cur_symtab = new SymbolTable(NULL, NULL);
	cur_type = t;

	apply(t);

	cur_type = NULL;

	delete off_thunked;
	delete off_pure;
	off_pure = NULL;
	off_thunked = NULL;
	assert (union_c == 0);
	assert (weak_c == 0);

	ret = cur_symtab;
	cur_symtab = NULL;

	ptu = new PhysTypeUser(t, ret_pt);
	ret_pt = NULL;

	return ret;
}

void SymTabBuilder::updateOffset(Expr* length)
{
	if (union_c > 0) {
		/* unions quelch updates until the last moment*/
		return;
	}

	off_pure = new AOPAdd(off_pure, length->simplify());
	off_thunked = new AOPAdd(off_thunked, length);

//	const EvalCtx	ectx(*cur_symtab, symtabs_thunked, constants);
//	off_thunked = evalReplace(ectx, off_thunked);
//	off_pure = evalReplace(ectx, off_pure);
}

void SymTabBuilder::visit(const TypeDecl* td)
{
	const Id		*scalar;
	const IdArray		*array;
	PhysicalType		*pt;

	if ((scalar = td->getScalar()) != NULL) {
		PhysTypeThunk	*ptt;

		pt = resolve_by_id_thunk(tm, td->getType());
		if (pt == NULL)
			return;

		if ((ptt = dynamic_cast<PhysTypeThunk*>(pt)) != NULL) {
			ExprList	*exprs;
			bool		set_success;

			exprs = new ExprList();
			exprs->add(off_thunked->copy());

			set_success = ptt->setArgs(exprs);
			assert (set_success);
		}

	} else if ((array = td->getArray()) != NULL) {
		PhysicalType	*base;
		PhysTypeThunk	*thunk_base;
		Expr		*e, *e_tmp;
		EvalCtx		ectx(*cur_symtab, symtabs_thunked, constants);

		base = resolve_by_id_thunk(tm, td->getType());
		if (base == NULL) return;

		e = (array->getIdx())->simplify();
		e_tmp = e->rewrite(&from_base_fc, off_pure);
		if (e_tmp != NULL) {
			delete e;
			e = e_tmp;
		}
		e = evalReplace(ectx, e);

		if ((thunk_base = dynamic_cast<PhysTypeThunk*>(base)) != NULL) {
			ExprList	*exprs;
			bool		set_success;

			exprs = new ExprList();
			off_thunked = evalReplace(ectx, off_thunked);
			exprs->add(off_thunked->copy());
			set_success = thunk_base->setArgs(exprs);
		}

		pt = new PhysTypeArray(base, e);
	} else {
		/* static decl must map to either an array or an id */
		assert (0 == 1);
	}

	cur_symtab->add(td->getName(), pt, off_thunked->simplify(), (weak_c > 0));
	retType(pt->copy());
	updateOffset(pt->getBits());
}

void SymTabBuilder::retType(PhysicalType* pt)
{
	assert (ret_pt != pt);
	if (ret_pt != NULL) {
		delete ret_pt;
	}
	ret_pt = pt;
}

void SymTabBuilder::visit(const TypeUnion* tu)
{
	TypeBlock::const_iterator		it;
	const TypeBlock				*block;
	PhysTypeUnion				*pt_union;
	PhysicalType				*ret;
	string					symname;

	union_c++;
	weak_c++;
	block = tu->getBlock();
	pt_union = new PhysTypeUnion("__union");
	for (it = block->begin(); it != block->end(); it++) {
		(*it)->accept(this);
		if (ret_pt == NULL) {
			continue;
		}

		pt_union->add(ret_pt->copy());
	}

	union_c--;
	weak_c--;

	if (tu->isArray()) {
		EvalCtx	ectx(*cur_symtab, symtabs_thunked, constants);
		Expr	*len, *len_tmp;

		len = tu->getArray()->getIdx()->simplify();
		len_tmp = len->rewrite(&from_base_fc, off_pure);
		if (len_tmp != NULL) {
			delete len;
			len = len_tmp;
		}

		len = evalReplace(ectx, len);

		ret = new PhysTypeArray(pt_union, len);
		symname = tu->getArray()->getName();
	} else {
		ret = pt_union;
		symname = tu->getScalar()->getName();
	}

	cur_symtab->add(symname, ret->copy(), off_thunked->simplify(), (weak_c > 0));
	updateOffset(ret->getBits());
	retType(ret);
}

void SymTabBuilder::visit(const TypeParamDecl* tp)
{
	/* directly resolving this is impossible, that's why we set the phys
	 * type as a thunk for later resolution */
	ptype_map::const_iterator	it;
	PhysicalType			*pt;
	PhysTypeThunk			*thunk;
	ExprList			*arg_copy;
	const string&			type_name(tp->getType()->getName());

	/* get the thunk */
	it = tm.find(string("thunk_") + type_name);
	if (it == tm.end()) {
		cerr	<< "Could not resolve thunk: \"" << type_name << "\""
			<< endl;
		return;
	}

	/* into the right type.. */
	pt = ((*it).second)->copy();
	thunk = dynamic_cast<PhysTypeThunk*>(pt);
	assert (thunk != NULL);

	/* set the args */
	/* XXX add support for from_base in args */
	arg_copy = (tp->getType()->getExprs())->copy();
	if (thunk->setArgs(arg_copy) == false) {
		cerr << "Thunk arg length mismatch on " 
		     << thunk->getType()->getName() << endl;
		delete arg_copy;
		delete thunk;
		return;
	}

	/* if it's an array, we need to wrap in an array type */
	if (tp->getArray() != NULL) {
		/* XXX add support for from_base */
		pt = new PhysTypeArray(pt, (tp->getArray()->getIdx())->copy());
	}

	cur_symtab->add(tp->getName(), pt, off_thunked->simplify(), (weak_c > 0));
	updateOffset(pt->getBits());
	retType(pt->copy());
}

void SymTabBuilder::visit(const TypeBlock* tb)
{
	TypeBlock::const_iterator	it;
	PhysTypeAggregate		*ret;

	ret = new PhysTypeAggregate("__block");

	for (	TypeBlock::const_iterator it = tb->begin(); 
		it != tb->end(); 
		it++) 
	{
		(*it)->accept(this);
		if (ret_pt == NULL) {
			continue;
		}

		ret->add(ret_pt->copy());
	}

	/* no need to updateOffset-- internal fields should have done it.. */

	/* but we should return the ptype.. */
	retType(ret);
}

void SymTabBuilder::visit(const TypeCond* tc)
{
	PhysicalType	*t, *f;
	Expr		*old_off_pure, *old_off_thunked;

	weak_c++;

	old_off_pure = off_pure->simplify();
	old_off_thunked = off_thunked->simplify();

	(tc->getTrueStmt())->accept(this);
	t = ret_pt->copy();

	delete off_pure;
	delete off_thunked;
	off_pure = old_off_pure;
	off_thunked = old_off_thunked;

	old_off_pure = off_pure->simplify();
	old_off_thunked = off_thunked->simplify();
	if (tc->getFalseStmt() != NULL) {
		tc->getFalseStmt()->accept(this);
		f = ret_pt->copy();
	} else {
		f = NULL;
	}

	delete off_pure;
	delete off_thunked;
	off_pure = old_off_pure;
	off_thunked = old_off_thunked;

	weak_c--;

	retType(cond_resolve(tc->getCond(), tm, t, f));
	
	if (ret_pt == NULL)
		return;

	updateOffset(ret_pt->getBits());
}

void SymTabBuilder::visit(const TypeFunc* tf)
{
	PhysicalType	*ret;
	string		func_name;
	ExprList	*args, *args_tmp;
	const FCall	*fcall;

	fcall = tf->getFCall();
	func_name = fcall->getName();

	/* rewrite arguments */
	args = (fcall->getExprs())->simplify();
	args->rewrite(&from_base_fc, off_pure);
	args_tmp = args->simplify();
	delete args;
	args = args_tmp;
	if (func_name == "align") {
		if (args->size() != 1) {
			cerr 	<< tf->getLineNo() << 
				": align takes exactly one argument" << endl;
			ret = NULL;
		} else {
			Expr	*align_pad_bits;

			align_pad_bits = new AOPMod(
				new AOPSub(
					new AOPMul(
					(args->front())->simplify(),
					new Number(8)),
					off_pure->simplify()),
				new AOPMul(
					(args->front())->simplify(),
					new Number(8)));

			/* have a proper offset in the binding, align it.. */
			/* note: aligning is (align_v - offset) % align_v */
			ret = new PhysTypeArray(new U1(), align_pad_bits);
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
		ret = genAssertEq(args, tf->getLineNo());
	} else if (func_name == "assert_le") {
		ret = genAssertLE(args, tf->getLineNo());
	} else {
		/* don't know what to do with it! */
		ret = new PhysTypeFunc(
			new FCall(
				new Id(fcall->getName()), 
				args->copy()));
	}

done:
	delete args;

	updateOffset(ret->getBits());
	retType(ret);
}

