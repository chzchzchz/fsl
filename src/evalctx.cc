#include "eval.h"
#include "func.h"
#include "runtime_interface.h"

extern const Func	*gen_func;
extern func_map		funcs_map;
extern type_map		types_map;
extern RTInterface	rt_glue;

using namespace std;

Expr* EvalCtx::resolve(const Id* id) const
{
	const_map::const_iterator	const_it;
	const SymbolTableEnt		*st_ent;
	const Type			*t;

	assert (id != NULL);

	/* is it a constant? */
	const_it = constants.find(id->getName());
	if (const_it != constants.end()) {
		return ((*const_it).second)->simplify();
	}
	
	
	if (cur_scope != NULL) {
		/* is is in the current scope? */
		if ((st_ent = cur_scope->lookup(id->getName())) != NULL) {
			const ThunkField	*tf;
			Expr*			offset;
			ExprList*		exprs;

			tf = st_ent->getFieldThunk();
			offset = tf->getOffset()->copyFCall();
			if (tf->getType() != NULL)
				return offset;

			return rt_glue.getLocal(
				offset, tf->getSize()->copyFCall());
		}
	} else if (func_args != NULL) {
		if (func_args->hasField(id->getName())) {
			return id->copy();
		}
	}

	/* support for access of dynamic types.. gets base bits for type */
	if ((t = typeByName(id->getName())) != NULL) {
		return rt_glue.getDyn(t);
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
	const SymbolTableEnt	*st_ent;
	const Type		*cur_type;
	const ThunkField	*thunk_field;
	Expr			*ret;

	first_ids_expr = *it_begin;
	first_idx = NULL;

	if (toName(first_ids_expr, first_name, first_idx) == false) {
		return NULL;
	}

	if ((st_ent = first_symtab->lookup(first_name)) == NULL) {
		if (first_idx != NULL) delete first_idx;
		return NULL;
	}

	/* XXX this isn't quite right.. need a fully qualified type. */
	ret = new Id(first_name);

	thunk_field = st_ent->getFieldThunk();
	cur_type = thunk_field->getType();
	if (thunk_field->getElems()->isSingleton() == false) {
		/* it's an array */
		if (first_idx == NULL) {
			cerr << "Array type but no index?" << endl;
			delete ret;
			return NULL;
		}

		ret = new AOPAdd(ret, 
			new AOPMul(
				thunk_field->getSize()->copyFCall(),
				first_idx));
	} else {
		/* it's a scalar, no need to update base */
		if (first_idx != NULL) {
			cerr << "Tried to index a scalar?" << endl;
			delete first_idx;
			delete ret;
			return NULL;
		}
	}

	if (cur_type == NULL) {
		cout << "NULL PTU ON ";
		first_ids_expr->print(cout);
		cout << endl;
	}

	assert (cur_type != NULL);

	next_symtab = symtabByName(cur_type->getName());

	return ret;
}

Expr* EvalCtx::getStructExpr(
	const Expr			*base,
	const SymbolTable		*first_symtab,
	const IdStruct::const_iterator	ids_first,
	const IdStruct::const_iterator	ids_end,
	const SymbolTableEnt*		&last_sym) const
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

	return buildTail(it, ids_end, ret, cur_symtab, last_sym);
}

Expr* EvalCtx::buildTail(
	IdStruct::const_iterator	it,
	IdStruct::const_iterator	ids_end,
	Expr*				ret,
	const SymbolTable*		parent_symtab,
	const SymbolTableEnt*		&last_sym) const
{
	const Expr		*cur_ids_expr;
	std::string		cur_name;
	const SymbolTableEnt	*st_ent;
	Expr			*ret_begin;
	const Type		*cur_type;
	Expr			*cur_idx = NULL;
	const SymbolTable	*cur_symtab;
	const ThunkField	*thunk_field;

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

	if ((st_ent = parent_symtab->lookup(cur_name)) == NULL) {
		cerr 	<< "Whoops! couldn't find " << cur_name 
			<< " in symtab" << endl;
		goto err_cleanup;
	}

	last_sym = st_ent;
	thunk_field = st_ent->getFieldThunk();
	cur_type = thunk_field->getType();

	/* for (REST OF IDS).(parent).(it) we want to compute 
	 * &(REST OF IDS) + offset(REST OF IDS, parent) + offset(parent, it)
	 * currently we only have
	 * ret = &(REST OF IDS) + offset(REST OF IDS, parent) 
	 * so we do
	 * ret += offset(parent, it)
	 * in this stage
	 */
	if (thunk_field->getElems()->isSingleton() == false) {
		Expr	*new_base;

		/* it's an array */
		if (cur_idx == NULL) {
			cerr << "Array type but no index?" << endl;
			goto err_cleanup;
		}

		new_base = Expr::rewriteReplace(
				thunk_field->getOffset()->copyFCall(),
				rt_glue.getThunkArg(),
				ret->simplify());

		new_base = new AOPAdd(
			new_base, 
			new AOPMul(
				Expr::rewriteReplace(
					thunk_field->getSize()->copyFCall(),
					rt_glue.getThunkArg(),
					ret->simplify()),
				evalReplace(*this, cur_idx->simplify())));

		delete ret;
		ret = new_base;

		delete cur_idx;
		cur_idx = NULL;
	} else {
		Expr	*new_base;

		/* it's a scalar */
		if (cur_idx != NULL) {
			cerr << "Tried to index a scalar?" << endl;
			goto err_cleanup;
		}

		new_base = Expr::rewriteReplace(
			thunk_field->getOffset()->copyFCall(),
			rt_glue.getThunkArg(),
			ret->simplify());

		delete ret;
		ret = new_base;
	}

	if (cur_type == NULL) {
		cur_symtab = NULL;
		delete ret_begin;
	} else {
		/* don't forget to replace any instances of the 
		 * used type with the name of the field being accessed!
		 * (where did we come from and where are we going?)
		 */
		ret = Expr::rewriteReplace(
			ret, 
			new Id(cur_type->getName()),
			ret_begin);
		ret_begin = NULL;

		cur_symtab = symtabByName(cur_type->getName());
	}

done:
	assert (cur_idx == NULL);
	return buildTail(++it, ids_end, ret, cur_symtab, last_sym);

err_cleanup:
	if (cur_idx != NULL) delete cur_idx;
	if (ret_begin != NULL) delete ret_begin;
	delete ret;

	return NULL;
}


Expr* EvalCtx::resolveCurrentScope(const IdStruct* ids) const
{
	Expr				*offset;
	const Id			*front_id;
	const SymbolTableEnt		*last_sym; 
	const ThunkField		*thunk_field;
	IdStruct::const_iterator	it;

	assert (cur_scope != NULL);
	assert (ids != NULL);

	it = ids->begin();
	assert (it != ids->end());

	/* in current scope? */
	offset = getStructExpr(
		NULL, cur_scope, ids->begin(), ids->end(), last_sym);
	if (offset == NULL)
		return NULL;

	/* talking about a user type.. just return the offset */
	/* XXX this should return a fully-qualified type */
	thunk_field = last_sym->getFieldThunk();
	if (thunk_field->getType() != NULL) {
		return offset;
	}

	return rt_glue.getLocal(offset, thunk_field->getSize()->copyFCall());
}

Expr* EvalCtx::resolveGlobalScope(const IdStruct* ids) const
{
	Expr				*offset;
	const Id			*front_id;
	ExprList			*exprs;
	IdStruct::const_iterator	it;
	const SymbolTable		*top_symtab;
	const SymbolTableEnt		*last_ste;
	const ThunkField		*thunk_field;
	const Type			*top_type;
	Expr				*base;

	it = ids->begin();
	front_id = dynamic_cast<const Id*>(*it);
	if (front_id == NULL) {
		cerr << "Expected ID ON GLOBALSCOPE" << endl;
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


	base = rt_glue.getDyn(top_type);

	it++;
	offset = getStructExpr(base, top_symtab, it, ids->end(), last_ste);
	delete base;

	if (offset == NULL) {
		return NULL;
	}

	thunk_field = last_ste->getFieldThunk();
	if (thunk_field->getType() != NULL) {
		/* pointer.. */
		return offset;
	}

	return rt_glue.getLocal(
		offset, thunk_field->getSize()->copyConstValue());
}

Expr* EvalCtx::resolveFuncArg(const IdStruct* ids) const
{
	Expr				*base;
	Expr				*offset;
	ExprList			*exprs;
	string				type_name;
	const Id*			first_id;
	const Type			*top_type;
	const SymbolTable		*top_symtab;
	const SymbolTableEnt		*last_ste;
	const ThunkField		*thunk_field;
	IdStruct::const_iterator	it;
	
	assert (func_args != NULL);

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

	if (func_args->lookupType(first_id->getName(), type_name) == false) {
		/* given argument name is not in argument list. */
		return NULL;
	}

	top_type = typeByName(type_name);
	top_symtab = symtabByName(type_name);
	if (top_symtab == NULL || top_type == NULL) {
		/* top_type == NULL => 
		* id struct should only be referencing user types-- no primitives!  */

		/* could not find type given by argslist */
		return NULL;
	}

	base = first_id->copy();
	it++;
	offset = getStructExpr(base, top_symtab, it, ids->end(), last_ste);
	delete base;

	if (offset == NULL) 
		return NULL;

	thunk_field = last_ste->getFieldThunk();

	/* only return offset if we are returning a user type */
	if (thunk_field->getType() != NULL)
		return offset;

	/* since we're not returning a user type, we know that the size to 
	 * read is going to be constant. don't need to bother with computing
	 * parameters for the size function-- inline it! */

	return rt_glue.getLocal(
		offset, thunk_field->getSize()->copyConstValue());
}

Expr* EvalCtx::resolve(const IdStruct* ids) const
{	
	Expr	*ret;

	if (cur_scope != NULL) {
		ret = resolveCurrentScope(ids);
		if (ret != NULL)
			return ret;
	}

	if (func_args != NULL) {
		ret = resolveFuncArg(ids);
		if (ret != NULL) {
			return ret;
		}
	}

	ret = resolveGlobalScope(ids);
	if (ret != NULL)
		return ret;

	/* can't resolve it.. */
	cerr << "Can't resolve idstruct: ";
	ids->print(cerr);
	cerr << endl;

	return NULL;
}

Expr* EvalCtx::resolveArrayInType(const IdArray* ida) const
{
	/* array is in current scope */
	const SymbolTableEnt		*st_ent;
	Expr				*evaled_idx;
	const ThunkField		*thunk_field;

	assert (cur_scope != NULL);

	if ((st_ent = cur_scope->lookup(ida->getName())) == NULL)
		return NULL;

	evaled_idx = eval(*this, ida->getIdx());
	if (evaled_idx == NULL) {
		cerr << "Could not resolve ";
		(ida->getIdx())->print(cerr);
		cerr << endl;
		return NULL;
	}

	thunk_field = st_ent->getFieldThunk();

	/* convert into __getLocalArray call */
	return rt_glue.getLocalArray(
		evaled_idx,
		thunk_field->getSize()->copyFCall(),
		thunk_field->getOffset()->copyFCall(),
		new AOPMul(
			thunk_field->getSize()->copyFCall(),
			thunk_field->getElems()->copyFCall()));
}

Expr* EvalCtx::resolve(const IdArray* ida) const
{
	Expr				*ret;
	const_map::const_iterator	const_it;

	assert (ida != NULL);

	if (cur_scope != NULL) {
		ret = resolveArrayInType(ida);
		if (ret != NULL)
			return ret;
	}
	
/* TODO -- add support for constant arrays */
	assert (0 == 1);
	
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

	st = symtabByName(s);
	if (st == NULL)
		return NULL;

	return st->getOwnerType();
}

const Type* EvalCtx::getTypeId(const Id* id) const
{	
	assert (id != NULL);

	if (cur_scope != NULL) {
		const SymbolTableEnt*	st_ent;
		st_ent = cur_scope->lookup(id->getName());
		if (st_ent != NULL) {
			return types_map[st_ent->getTypeName()];
		}
	}

	if (func_args != NULL) {
		string	type_name;
		if (func_args->lookupType(id->getName(), type_name)) {
			return types_map[type_name];
		}
	}

	return types_map[id->getName()];
}

const Type* EvalCtx::getTypeIdStruct(const IdStruct* ids) const
{
	IdStruct::const_iterator	it;
	const Type			*cur_type;

	it = ids->begin();
	cur_type = getType(*it);

	/* walk the symbol tables */
	for (++it; it != ids->end(); it++) {
		const Id		*id;
		const IdArray		*ida;
		const SymbolTable	*cur_symtab;
		const SymbolTableEnt	*cur_st_ent;
		string			fieldname;

		if (cur_type == NULL)
			return NULL;

		cur_symtab = symtabByName(cur_type->getName());
		if (cur_symtab == NULL)
			return NULL;

		if ((id = dynamic_cast<const Id*>(*it)) != NULL) {
			fieldname = id->getName();
		} else if ((ida = dynamic_cast<const IdArray*>(*it)) != NULL) {
			fieldname = ida->getName();
		} else {
			/* id or ida expected */
			return NULL;
		}

		cur_st_ent = cur_symtab->lookup(fieldname);
		if (cur_st_ent == NULL) {
			/* field not found in symtab */
			return NULL;
		}

		cur_type = cur_st_ent->getFieldThunk()->getType();
	}

	return cur_type;
}

/* returns the type given by an expression */
const Type* EvalCtx::getType(const Expr* e) const
{
	const FCall	*fc;
	const Id	*id;
	const IdArray	*ida;
	const IdStruct	*ids;

	assert (e != NULL);

	fc = dynamic_cast<const FCall*>(e);
	if (fc != NULL) {
		const Func*	f;

		f = funcs_map[fc->getName()];
		if (f == NULL) {
			return NULL;
		}

		return types_map[f->getRet()];
	}

	id = dynamic_cast<const Id*>(e);
	if (id != NULL)
		return getTypeId(id);

	ida = dynamic_cast<const IdArray*>(e);
	if (ida != NULL) {
		Id	tmp_id(ida->getName());
		return getTypeId(&tmp_id);
	}

	ids = dynamic_cast<const IdStruct*>(e);
	if (ids != NULL) {
		return getTypeIdStruct(ids);
	}


	/* arith expression */
	cout << "WTF????????" << endl;
	return NULL;
}

