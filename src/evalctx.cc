#include "eval.h"
#include "func.h"

extern const Func	*gen_func;

using namespace std;

Expr* EvalCtx::resolve(const Id* id) const
{
	const_map::const_iterator	const_it;
	sym_binding			symbind;
	const Type			*t;

	assert (id != NULL);

	/* is it a constant? */
	const_it = constants.find(id->getName());
	if (const_it != constants.end()) {
		return ((*const_it).second)->simplify();
	}
	
	
	/* is is in the current scope? */
	if (cur_scope.lookup(id->getName(), symbind) == true) {
	#if 0
		ExprList	*elist;

		elist = new ExprList();
		elist->add(symbind_off(symbind)->simplify());
		elist->add(symbind_phys(symbind)->getBits());

		return new FCall(new Id("__getLocal"), elist);
	#endif
		return symbind_off(symbind)->simplify();
	}

	/* support for access of dynamic types.. gets base bits for type */
	if ((t = typeByName(id->getName())) != NULL) {
		ExprList	*exprs;

		exprs = new ExprList();
		exprs->add(new Number(t->getTypeNum()));
		return new FCall(new Id("__getDyn"), exprs);
	}

	/* could not resolve */
	return NULL;
}

Expr* EvalCtx::getStructExprBase(
	const SymbolTable		*first_symtab,
	const IdStruct::const_iterator	it_begin,
	const SymbolTable*		&next_symtab) const
{
	const Expr		*first_ids_expr;
	string			first_name;
	Expr			*first_idx;
	sym_binding		sb;
	PhysicalType		*pt;
	PhysTypeArray		*pta;
	const PhysTypeUser	*ptu;
	const Type		*cur_type;
	Expr			*ret;

	first_ids_expr = *it_begin;
	first_idx = NULL;

	if (toName(first_ids_expr, first_name, first_idx) == false) {
		return NULL;
	}

	if (first_symtab->lookup(first_name, sb) == false) {
		if (first_idx != NULL) delete first_idx;
		return NULL;
	}

	ret = new Id(first_name);

	pt = symbind_phys(sb);
	pta = dynamic_cast<PhysTypeArray*>(pt);
	if (pta != NULL) {
		Expr	*tmp_idx;

		/* it's an array */
		if (first_idx == NULL) {
			cerr << "Array type but no index?" << endl;
			delete ret;
			return NULL;
		}

		tmp_idx = new AOPSub(first_idx, new Number(1));
		ret = new AOPAdd(ret, pta->getBits(tmp_idx));

		delete tmp_idx;

		cur_type = PT2Type(pta->getBase());
		ptu = dynamic_cast<const PhysTypeUser*>(pta->getBase());
	} else {
		/* it's a scalar */
		if (first_idx != NULL) {
			cerr << "Tried to index a scalar?" << endl;
			delete first_idx;
			delete ret;
			return NULL;
		}
		cur_type = PT2Type(pt);
	}

	if (cur_type == NULL) {
		cout << "NULL PTU ON ";
		first_ids_expr->print(cout);
		cout << endl;
	}

	assert (cur_type != NULL);

	cerr << "WHAT LUCK: TYPENAME = " << cur_type->getName() << endl;
	next_symtab = symtabByName(cur_type->getName());

	cerr << "getStructExprBase DONE" << endl;

	return ret;
}

Expr* EvalCtx::getStructExpr(
	const Expr			*base,
	const SymbolTable		*first_symtab,
	const IdStruct::const_iterator	ids_first,
	const IdStruct::const_iterator	ids_end,
	const PhysicalType*		&final_type) const
{
	IdStruct::const_iterator	it;
	Expr				*ret;
	const SymbolTable		*cur_symtab;

	it = ids_first;
	if (base == NULL) {
		ret = getStructExprBase(first_symtab, it, cur_symtab);
		if (ret == NULL) {
			return NULL;
		}
		it++;
	} else {
		ret = base->simplify();
		cur_symtab = first_symtab;
	}

	return buildTail(it, ids_end, ret, cur_symtab, final_type);
}

Expr* EvalCtx::buildTail(
	IdStruct::const_iterator	it,
	IdStruct::const_iterator	ids_end,
	Expr*				ret,
	const SymbolTable*		parent_symtab,
	const PhysicalType*		&final_type) const
{
	const Expr		*cur_ids_expr;
	std::string		cur_name;
	PhysicalType		*cur_pt;
	PhysTypeArray		*pta;
	sym_binding		sb;
	Expr			*ret_begin;
	const Type		*cur_type;
	Expr			*cur_idx = NULL;
	const PhysicalType	*pt_base;
	const SymbolTable	*cur_symtab;

	assert (ret != NULL);
	
	if (it == ids_end)
		return ret;

	ret_begin = ret->simplify();

	if (parent_symtab == NULL) {
		/* no way we could possibly resolve the current 
		 * element. */
		goto err_cleanup;
	}

	cur_ids_expr = *it;

	/* get current name of id/idarray in struct and its index (if any) */
	if (toName(cur_ids_expr, cur_name, cur_idx) == false) {
		goto err_cleanup;
	}

	if (parent_symtab->lookup(cur_name, sb) == false) {
		goto err_cleanup;
	}

	cur_pt = symbind_phys(sb);
	cur_type = PT2UserTypeDrill(cur_pt);
	pt_base = PT2Base(cur_pt);

	/* for (REST OF IDS).(parent).(it) we want to compute 
	 * &(REST OF IDS) + offset(REST OF IDS, parent) + offset(parent, it)
	 * currently we only have
	 * ret = &(REST OF IDS) + offset(REST OF IDS, parent) 
	 * so we do
	 * ret += offset(parent, it)
	 * in this stage
	 */
	pta = dynamic_cast<PhysTypeArray*>(cur_pt);
	if (pta != NULL) {
		/* it's an array */
		if (cur_idx == NULL) {
			cerr << "Array type but no index?" << endl;
			goto err_cleanup;
		}

		cerr << "IN AN ARRAY!!!!" << endl;
		ret = new AOPAdd(ret, symbind_off(sb)->simplify());
		ret = new AOPAdd(ret, pta->getBits(cur_idx));
		delete cur_idx;
		cur_idx = NULL;
	} else {
		/* it's a scalar */
		if (cur_idx != NULL) {
			cerr << "Tried to index a scalar?" << endl;
			goto err_cleanup;
		}

		cerr <<"SCALAR: ";
		symbind_off(sb)->print(cerr);
		cerr << endl;

		/* right now we are generating the total size of some type...
		 * if the size relies on a thunked symbol, then that thunked symbol must
		 * know where its base is, hence we have B_THIS_IS_THUNK_ARG, which will
		 * take the expression up until now as the base. */
		cerr << "REWRITING SCALAR. " << endl;
		ret = new AOPAdd(
			ret, 
			Expr::rewriteReplace(
				symbind_off(sb)->simplify(),
				new Id("PT_THUNK_ARG"),
				ret->simplify()));
		cerr << "DONE REWRITING SCALAR. " << endl;

	}

	if (cur_type == NULL) {
		cur_symtab = NULL;
		delete ret_begin;
	} else {
		/* don't forget to replace any instances of the 
		 * used type with the name of the field being accessed!
		 * (where did we come from and where are we going?)
		 */

		cerr << "REPLACING " << cur_type->getName() << endl;
		ret = Expr::rewriteReplace(
			ret, 
			new Id(cur_type->getName()),
			ret_begin);
		ret_begin = NULL;

		cur_symtab = symtabByName(pt_base->getName());
	}

	final_type = cur_pt;

done:
	assert (cur_idx == NULL);
	return buildTail(++it, ids_end, ret, cur_symtab, final_type);

err_cleanup:
	if (cur_idx != NULL) delete cur_idx;
	if (ret_begin != NULL) delete ret_begin;
	delete ret;

	return NULL;
}


Expr* EvalCtx::resolveCurrentScope(const IdStruct* ids) const
{
	Expr				*offset;
	const PhysicalType		*ids_pt;
	const Id			*front_id;
	ExprList			*exprs;
	const PhysTypeArray		*pta;
	IdStruct::const_iterator	it;

	assert (ids != NULL);

	it = ids->begin();
	assert (it != ids->end());

	/* in current scope? */
	offset = getStructExpr(
		NULL,
		&cur_scope, ids->begin(), ids->end(), ids_pt);
	if (offset == NULL)
		return NULL;

	if (isPTaUserType(ids_pt) == true)
		return offset;

	pta = dynamic_cast<const PhysTypeArray*>(ids_pt);

	exprs = new ExprList();
	exprs->add(offset->simplify());

	if (pta != NULL)
		exprs->add((pta->getBase())->getBits());
	else
		exprs->add(ids_pt->getBits());

	delete offset;

	return new FCall(new Id("__getLocal"), exprs);
}

Expr* EvalCtx::resolveGlobalScope(const IdStruct* ids) const
{
	Expr				*offset;
	const PhysicalType		*ids_pt;
	const Id			*front_id;
	ExprList			*exprs;
	const PhysTypeArray		*pta;
	IdStruct::const_iterator	it;
	const SymbolTable		*top_symtab;
	const Type			*top_type;
	Expr				*base;
	ExprList			*base_exprs;

	it = ids->begin();
	front_id = dynamic_cast<const Id*>(*it);
	if (front_id == NULL) {
		cerr << "NOT AN ID ON GLOBALSCOPE" << endl;
		return NULL;
	}

	top_type = typeByName(front_id->getName());
	top_symtab = symtabByName(front_id->getName());
	if (top_symtab == NULL || top_type == NULL) {
		/* could not find in global scope.. */
		if (top_type == NULL) cerr << "COULD NOT FIND TOP_TYPE" << endl;
		if (top_symtab == NULL) cerr << "COULD NOT FIND TOP_ST" << endl;
		return NULL;
	}

	base_exprs = new ExprList();
	base_exprs->add(new Number(top_type->getTypeNum()));
	base = new FCall(new Id("__getDyn"), base_exprs);

	it++;
	offset = getStructExpr(base, top_symtab, it, ids->end(), ids_pt);
	delete base;

	if (offset == NULL) {
		cerr << "NULLOFFSET" << endl;
		return NULL;
	}

	if (isPTaUserType(ids_pt) == true) {
		/* pointer.. */
		return offset;
	}


	pta = dynamic_cast<const PhysTypeArray*>(ids_pt);

	exprs = new ExprList();
	exprs->add(offset->simplify());
	if (pta != NULL)
		exprs->add((pta->getBase())->getBits());
	else
		exprs->add(ids_pt->getBits());

	delete offset;

	return new FCall(new Id("__getLocal"), exprs);
}

Expr* EvalCtx::resolveFuncArg(const IdStruct* ids) const
{
	const ArgsList			*args;
	Expr				*base;
	Expr				*offset;
	ExprList			*exprs;
	const Id*			type_name;
	const Id*			first_id;
	const Type			*top_type;
	const SymbolTable		*top_symtab;
	const PhysicalType		*ids_pt;
	const PhysTypeArray		*pta;
	IdStruct::const_iterator	it;
	
	if (gen_func == NULL)
		return NULL;

	if ((args = gen_func->getArgs()) == NULL)
		return NULL;

	it = ids->begin();
	if (dynamic_cast<const IdArray*>(*it) != NULL) {
		/* TODO -- array of idstructs in func argument */
		cerr 	<< "Array of IdStructs on func argument not supported."
			<< endl;
		return NULL;
	}

	first_id = dynamic_cast<const Id*>(*it);
	if (first_id == NULL)
		return NULL;

	if ((type_name = args->findType(first_id->getName())) == NULL) {
		/* given argument name is not in argument list. */
		return NULL;
	}

	top_type = typeByName(type_name->getName());
	if (top_type == NULL) {
		/* id struct should only be referencing user types-- no primitives!  */
	}

	top_symtab = symtabByName(type_name->getName());
	if (top_symtab == NULL || top_type == NULL) {
		/* could not find type given by argslist */
		return NULL;
	}

	base = first_id->copy();
	it++;
	offset = getStructExpr(base, top_symtab, it, ids->end(), ids_pt);
	delete base;

	if (offset == NULL) {
		return NULL;
	}
	

	if (isPTaUserType(ids_pt) == true)
		return offset;

	pta = dynamic_cast<const PhysTypeArray*>(ids_pt);

	exprs = new ExprList();
	exprs->add(offset->simplify());
	if (pta != NULL)
		exprs->add((pta->getBase())->getBits());
	else
		exprs->add(ids_pt->getBits());

	delete offset;

	return new FCall(new Id("__getLocal"), exprs);
}

Expr* EvalCtx::resolve(const IdStruct* ids) const
{	
	Expr	*ret;

	cerr << "Trying to resolve by curscope" << endl;
	ret = resolveCurrentScope(ids);
	if (ret != NULL)
		return ret;

	cerr << "Resolve global scope" << endl;
	ret = resolveGlobalScope(ids);
	if (ret != NULL)
		return ret;

	cerr << "Resolve Func Arg" << endl;
	ret = resolveFuncArg(ids);
	if (ret != NULL)
		return ret;

	/* can't resolve it.. */
	cerr << "Can't resolve idstruct: ";
	ids->print(cerr);
	cerr << endl;

	return NULL;
}

Expr* EvalCtx::resolve(const IdArray* ida) const
{
	const_map::const_iterator	const_it;
	sym_binding			symbind;
	const PhysicalType		*pt;
	const PhysTypeArray		*pta;

	assert (ida != NULL);

	if (cur_scope.lookup(ida->getName(), symbind) == true) {
		/* array is in current scope */
		ExprList	*elist;
		Expr		*evaled_idx;

		ida->getIdx()->print(cout);
		cout << endl;

		evaled_idx = eval(*this, ida->getIdx());

		if (evaled_idx == NULL) {
			cerr << "Could not resolve ";
			(ida->getIdx())->print(cerr);
			cerr << endl;
			return NULL;
		}

		pt = symbind_phys(symbind);
		assert (pt != NULL);

		pta = dynamic_cast<const PhysTypeArray*>(pt);
		assert (pta != NULL);


		/* convert into __getLocalArray call */
		elist = new ExprList();
		elist->add(evaled_idx->simplify());
		elist->add((pta->getBase())->getBits());
		elist->add(symbind_off(symbind)->simplify());
		elist->add(symbind_phys(symbind)->getBits());

		delete evaled_idx;

		return new FCall(new Id("__getLocalArray"), elist);
	}
	
/* TODO -- add support for constant arrays */
	assert (0 == 1);
#if 0
	const_it = constants.find(id->getName());
	if (const_it != constants.end()) {
		return (const_it.second)->simplify();
	}
#endif
	
	
	/* could not resolve */
	return NULL;
}

bool EvalCtx::toName(const Expr* e, std::string& in_str, Expr* &idx) const
{
	const Id		*id;
	const IdArray		*ida;
	
	idx = NULL;
	id = dynamic_cast<const Id*>(e);
	if (id != NULL) {
		in_str = id->getName();
		return true;
	}

	ida = dynamic_cast<const IdArray*>(e);
	if (ida != NULL) {
		in_str = ida->getName();
		idx = (ida->getIdx())->simplify();
		return true;
	}

	return false;
}

const SymbolTable* EvalCtx::symtabByName(const std::string& s) const
{
	symtab_map::const_iterator	it;

	it = all_types.find(s);
	if (it == all_types.end())
		return NULL;

	return (*it).second;
}

const Type* EvalCtx::typeByName(const std::string& s) const
{
	const SymbolTable	*st;
	const Type		*t;
	const PhysicalType	*pt;


	st = symtabByName(s);
	if (st == NULL)
		return NULL;

	return st->getOwnerType();
}
