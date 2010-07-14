#include <iostream>

#include "symtab.h"
#include "type.h"
#include "cond.h"
#include "expr.h"

using namespace std;

extern const_map		constants;
extern symtab_map		symtabs_thunked;

static FCall	from_base_fc(new Id("from_base"), new ExprList());

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

	cur_symtab->add(td->getName(), pt, off_thunked->simplify());
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

	if (tu->isArray()) {
		EvalCtx	ectx(*cur_symtab, symtabs_thunked, constants);
		Expr	*len, *len_tmp;

		len = tu->getArray()->getIdx()->simplify();
		len_tmp = len->rewrite(&from_base_fc, off_pure);
		if (len_tmp != NULL) {
			delete len;
			len = len_tmp;
		}

		cerr << "DOING EVALREPLACE" << endl;
		len = evalReplace(ectx, len);

		cerr << "I HAVE OUR LEN = ";
		len->print(cerr);
		cerr << endl;

		ret = new PhysTypeArray(pt_union, len);
		symname = tu->getArray()->getName();
	} else {
		ret = pt_union;
		symname = tu->getScalar()->getName();
	}

	cur_symtab->add(symname, ret->copy(), off_thunked->simplify());
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

	cur_symtab->add(tp->getName(), pt, off_thunked->simplify());
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

bool SymbolTable::loadArgs(const ptype_map& tm, const ArgsList *args)
{
	if (args == NULL) return true;

	for (unsigned int i = 0; i < args->size(); i++) {
		pair<Id*, Id*>  p;
		PhysicalType    *pt;
		bool            rc;

		p = args->get(i);
		pt = resolve_by_id(tm, p.first);
		if (pt == NULL) return false;


//		rc = add(
//			(p.second)->getName(),
//			pt,
//			new FCall(new Id("__arg"), exprs));
//
		rc = add((p.second)->getName(), pt, (p.second)->copy());


		if (rc == false) return false;
	}

	return true;
}


SymbolTable* SymTabThunkBuilder::getSymTab(const TypeBlock* tb, PhysicalType* &ptt)
{
	SymbolTable	*ret;

	assert (cur_symtab == NULL);
	assert (ret_pt == NULL);

	cur_symtab = new SymbolTable(NULL, NULL);
	cur_type = tb->getOwner();

	visit(tb);

	ret = cur_symtab;
	cur_symtab = NULL;

	ptt = ret_pt;
	cur_type = NULL;

	return ret;
}

SymbolTable* SymTabThunkBuilder::getSymTab(const Type* t, PhysicalType* &ptt)
{
	SymbolTable	*ret;

	assert (cur_symtab == NULL);
	assert (ret_pt == NULL);

	cur_symtab = new SymbolTable(NULL, NULL);
	cur_type = t;

	apply(t);

	ret = cur_symtab;
	cur_symtab = NULL;
	if (ret_pt != NULL) {
		delete ret_pt;
		ret_pt = NULL;
	}

	ptt = new PhysTypeThunk(
		cur_type,
		new ExprList(new Id("NEEDS_THUNK_VAL")));
	cur_type = NULL;

	return ret;
}

void SymTabThunkBuilder::retType(PhysicalType* pt)
{
	if (ret_pt != NULL)
		delete ret_pt;
	ret_pt = pt;
}

extern void dump_ptypes();

void SymTabThunkBuilder::addToCurrentSymTab(
	const std::string& 	type_str,
	const std::string&	field_name,
	const Id*		scalar,
	const IdArray*		array,
	ExprList*		type_args)
{
	const Type		*t;
	PhysicalType		*passed_pt;
	FCall			*passed_offset_fcall;
	ExprList		*exprs;
	const PhysicalType	*pt;


	t = tm.getUserType(type_str);
	pt = tm.getPT(type_str);

	if (t = tm.getUserType(type_str)) {
		if (type_args != NULL) {
			type_args->insert(type_args->begin(), new Id("XYZ_NEED_THUNK_V"));
		} else {
			type_args = new ExprList(new Id("XYZ_NEED_THUNK_V"));
		}
		passed_pt = new PhysTypeThunk(t, type_args);
		if (array != NULL) { 
			passed_pt = new PhysTypeArray(passed_pt, array->getIdx()->copy());
		}
	} else if (pt = tm.getPT(type_str)) {
		passed_pt = pt->copy();
		if (array != NULL) { 
			passed_pt = new PhysTypeArray(passed_pt, array->getIdx()->copy());
		}

		if (type_args != NULL) {
			cerr << "type args for a primitive type?? OOps" << endl;
			delete type_args;
		}
	} else {
		cout << "Couldn't resolve type string: " << type_str << endl;
		dump_ptypes();
		assert (0 == 1);
	}
	
	exprs = new ExprList(new Id("PT_THUNK_ARG"));
	for (int i = 0; i < cur_type->getNumArgs(); i++) {
		exprs->add(new Id("B_FILL_IN_THIS_ARG_ANTHONY"));
	}

	passed_offset_fcall = new FCall(
		new Id(typeOffThunkName(cur_type, field_name)),
		exprs);

	retType(passed_pt->copy());

	cur_symtab->add(field_name, passed_pt, passed_offset_fcall);
}

void SymTabThunkBuilder::addToCurrentSymTabAnonType(
	const PhysicalType*	pt,
	const std::string&	field_name,
	const Id*		scalar,
	const IdArray*		array)
{
	PhysicalType		*passed_pt;
	FCall			*passed_offset_fcall;
	ExprList		*exprs;

	assert (pt != NULL);


	passed_pt = pt->copy();
	if (array != NULL) { 
		passed_pt = new PhysTypeArray(passed_pt, array->getIdx()->copy());
	}
	
	exprs = new ExprList();
	exprs->add(new Id("A_THIS_IS_THUNK_ARG"));
	for (int i = 0; i < cur_type->getNumArgs(); i++) {
		exprs->add(new Id("A_FILL_IN_THIS_ARG_ANTHONY"));
	}

	passed_offset_fcall = new FCall(
		new Id(typeOffThunkName(cur_type, field_name)),
		exprs);

	retType(passed_pt->copy());

	cur_symtab->add(field_name, passed_pt, passed_offset_fcall);
}



void SymTabThunkBuilder::visit(const TypeDecl* td)
{
	addToCurrentSymTab(
		td->getType()->getName(),
		td->getName(), td->getScalar(), td->getArray(),
		NULL);
}

void SymTabThunkBuilder::visit(const TypeBlock* tb)
{
	TypeBlock::const_iterator	it;
	PhysTypeAggregate		*ret;

	ret = new PhysTypeAggregate("__block" + cur_type->getName());

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

	/* but we should return the ptype.. */
	retType(ret);
}

void SymTabThunkBuilder::visit(const TypeCond* tc)
{
	PhysicalType	*pt_t, *pt_f;

	(tc->getTrueStmt())->accept(this);
	pt_t = ret_pt->copy();

	if (tc->getFalseStmt() != NULL) {
		tc->getFalseStmt()->accept(this);
		pt_f = ret_pt->copy();
	} else {
		pt_f = NULL;
	}

	retType(cond_resolve(tc->getCond(), tm, pt_t, pt_f));
}


void SymTabThunkBuilder::visit(const TypeUnion* tu)
{
	TypeBlock::const_iterator	it;
	const TypeBlock			*block;
	PhysTypeUnion			*pt_anon_union;

	block = tu->getBlock();
	pt_anon_union = new PhysTypeUnion(
		"__union_"+cur_type->getName()+"_"+tu->getName());

	for (it = block->begin(); it != block->end(); it++) {
		(*it)->accept(this);
		if (ret_pt != NULL)
			continue;
		pt_anon_union->add(ret_pt);
	}

	SymTabThunkBuilder	*stb = new SymTabThunkBuilder(tm);
	SymbolTable		*union_symtab = NULL;
	PhysicalType		*union_ptt = NULL;

	union_symtab = stb->getSymTab(block, union_ptt);
	union_symtab->setOwner(
		new PhysTypeThunk(
			cur_type, 
			new ExprList(new Id("NEEDS_UNION_THUNK_V"))));
	symtabs_thunked[pt_anon_union->getName()] = union_symtab;
	delete union_ptt;
	delete stb;

	addToCurrentSymTabAnonType(
		pt_anon_union, 
		tu->getName(),
		tu->getScalar(),
		tu->getArray());

	delete pt_anon_union;
}



void SymTabThunkBuilder::visit(const TypeParamDecl* tp)
{
	addToCurrentSymTab(
		tp->getType()->getName(),
		tp->getName(), tp->getScalar(), tp->getArray(),
		(tp->getType()->getExprs())->copy());
}

