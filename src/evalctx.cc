#include "eval.h"
#include "func.h"
#include "runtime_interface.h"
#include <string.h>

extern const Func	*gen_func;
extern func_map		funcs_map;
extern type_map		types_map;
extern symtab_map	symtabs;
extern const_map	constants;
extern RTInterface	rt_glue;

using namespace std;

static Expr* replaceClosure(Expr* in_expr, Expr* diskoff, Expr* params)
{
	Expr	*ret = in_expr;

	ret = Expr::rewriteReplace(
		ret,
		rt_glue.getThunkClosure(),
		FCall::mkClosure(diskoff, params));

	ret = Expr::rewriteReplace(
		ret,
		rt_glue.getThunkArgIdx(),
		new Number(0));

	return ret;
}


Expr* EvalCtx::setNewOffsetsArray(
	Expr* new_base,
	Expr** new_params,
	const struct TypeBase& tb,
	const ThunkField* tf,
	const Expr* idx) const
{
	const Type	*t;
	const Type	*parent;

	assert (new_params != NULL && *new_params != NULL);

	/* no index? => scalar => error */
	if (idx == NULL) return NULL;

	parent = tb.tb_type;
	t = tf->getType();
	if (t != NULL) {
		/* non-constant type size-- call out to runtime */
		Expr	*array_elem_base;

		*new_params = Expr::rewriteReplace(
			*new_params, new Id("@"), idx->simplify());
		if (t->isUnion() == false) {
			array_elem_base = rt_glue.computeArrayBits(
				parent,
				tf->getFieldNum(),
				tb.tb_diskoff,
				tb.tb_parambuf,
				evalReplace(*this, idx->simplify()));
			array_elem_base = new AOPAdd(
				new_base, array_elem_base);
		} else {
			Expr	*sz;

			sz = new AOPMul(
				replaceClosure(
					tf->getSize()->copyFCall(),
					tb.tb_diskoff->simplify(),
					tb.tb_parambuf->simplify()),
				evalReplace(*this, idx->simplify()));
			array_elem_base = new AOPAdd(new_base, sz);
		}
		new_base = array_elem_base;
	} else {
		/* constant type size-- just multiply */
		Expr	*field_size;
		Expr	*array_off;

		field_size = replaceClosure(
			tf->getSize()->copyFCall(),
			tb.tb_diskoff->simplify(),
			tb.tb_parambuf->simplify());
		array_off = evalReplace(*this, idx->simplify());

		new_base = new AOPAdd(
			new_base, new AOPMul(field_size, array_off));
	}

	return new_base;
}

bool EvalCtx::setNewOffsets(
	struct TypeBase& tb, const Expr* idx) const
{
	const ThunkField	*tf;
	Expr			*new_base = NULL;
	Expr			*new_params = NULL;
	Expr			*offset_fc;

	assert (tb.tb_lastsym != NULL);
	assert (tb.tb_diskoff != NULL);
	assert (tb.tb_parambuf != NULL);

	tf = tb.tb_lastsym->getFieldThunk();
	assert (tf != NULL);

	/* compute base and params for base element */
	/* if this isn't an array, we're already done! */
	offset_fc = tf->getOffset()->copyFCall();
	assert (offset_fc != NULL);

	new_base = replaceClosure(
		offset_fc,
		tb.tb_diskoff->simplify(),
		tb.tb_parambuf->simplify());

	new_params = replaceClosure(
		tf->getParams()->copyFCall(),
		tb.tb_diskoff->simplify(),
		tb.tb_parambuf->simplify());

	if (tf->getElems()->isSingleton() == false) {
		new_base = setNewOffsetsArray(
			new_base, &new_params, tb, tf, idx);
		if (new_base == NULL) goto err_cleanup;
	} else {
		/* trying to index into a scalar type? */
		if (idx != NULL) goto err_cleanup;

		/* otherwise, we already have the offset+param we need... */
	}

	delete tb.tb_diskoff;
	delete tb.tb_parambuf;

	tb.tb_diskoff = new_base;
	tb.tb_parambuf = new_params;

	return true;

err_cleanup:
	if (new_base != NULL) delete new_base;
	if (new_params != NULL) delete new_params;

	return false;
}

bool EvalCtx::resolveTail(
	TypeBase			&tb,	/* current base */
	const IdStruct* 		ids,
	IdStruct::const_iterator	ids_begin) const
{
	string	name;
	Expr	*idx = NULL;

	assert (ids != NULL);

	/* nothing more to pass over */
	if (ids_begin == ids->end())
		return true;

	/* can only crawl if we're sitting on a type */
	if (tb.tb_type == NULL)
		return false;

	if (toName(*ids_begin, name, idx) == false)
		return false;

	tb.tb_lastsym = tb.tb_symtab->lookup(name);
	if (tb.tb_lastsym == NULL)
		return false;

	if (setNewOffsets(tb, idx) == false)
		goto cleanup_err;


	tb.tb_type = tb.tb_lastsym->getType();
	tb.tb_symtab = (tb.tb_type) ? symtabByName(tb.tb_type->getName()):NULL;

	ids_begin++;
	if (resolveTail(tb, ids, ids_begin) == false)
		goto cleanup_err;

	if (idx != NULL) delete idx;

	return true;

cleanup_err:
	if (tb.tb_diskoff != NULL) delete tb.tb_diskoff;
	if (tb.tb_parambuf != NULL) delete tb.tb_parambuf;
	if (idx != NULL) delete idx;

	tb.tb_diskoff = NULL;
	tb.tb_parambuf = NULL;

	return false;
}

bool EvalCtx::resolveIdStructCurScope(
		const IdStruct* ids,
		const std::string& name,
		const Expr* idx,
		struct TypeBase& tb) const
{
	if (cur_scope == NULL)
		return false;

	/* type scoped */
	/* two cases:
	 * 	1. reference to current type (e.g. ext2_inode.ino_num)
	 * 	2. reference to internal field (e.g. ino_num)
	 */
	tb.tb_lastsym = cur_scope->lookup(name);
	if (tb.tb_lastsym == NULL)
		return false;

	tb.tb_type = tb.tb_lastsym->getType();
	if (tb.tb_type != NULL)
		tb.tb_symtab = symtabByName(
			tb.tb_type->getName());
	else
		tb.tb_symtab = NULL;

	tb.tb_diskoff = rt_glue.getThunkArgOffset();
	tb.tb_parambuf = rt_glue.getThunkArgParamPtr();
	setNewOffsets(tb, idx);

	return resolveTail(tb, ids, ++(ids->begin()));
}

bool EvalCtx::resolveIdStructFunc(
		const IdStruct* ids,
		const std::string& name,
		const Expr* idx,
		struct TypeBase& tb) const
{
	/* function scoped */
	/* pull the typepass our of the function arguments */

	if (cur_func_blk == NULL)
		return false;

	tb.tb_lastsym = NULL;
	tb.tb_type = cur_func->getArgs()->getType(name);
	if (tb.tb_type == NULL)
		tb.tb_type = cur_func_blk->getVarType(name);

	if (tb.tb_type == NULL)
		return false;
	tb.tb_symtab = symtabByName(tb.tb_type->getName());
	tb.tb_diskoff = new FCall(
		new Id("__extractOff"),
		new ExprList(new Id(name)));
	tb.tb_parambuf = new FCall(
		new Id("__extractParam"),
		new ExprList(new Id(name)));
	return resolveTail(tb, ids, ++(ids->begin()));
}


Expr* EvalCtx::resolve(const IdStruct* ids) const
{
	const ThunkField		*thunk_field;
	const Type			*t;
	struct TypeBase			tb;
	string				name;
	Expr				*idx;
	Expr				*ret;
	Expr				*parent_closure;
	bool				found_expr;

	assert (ids != NULL);

	if (toName(ids->front(), name, idx) == false)
		return NULL;

	found_expr = false;

	found_expr = resolveIdStructCurScope(ids, name, idx, tb);
	if (found_expr) {
		/* closure is from parent type.. */
		parent_closure = rt_glue.getThunkClosure();
	}

	if (!found_expr && idx != NULL) {
		/* idx on top level only permitted for expressions 
		 * being resolved within the type scope */
		delete idx;
		cerr << "Idx on Func Arg." << endl;
		return NULL;
	}

	if (!found_expr) {
		found_expr = resolveIdStructFunc(ids, name, idx, tb);
		if (found_expr) {
			/* closure is from head of idstruct */
			parent_closure = new Id(name);
		}
	}

	if (!found_expr && (t = typeByName(name)) != NULL) {
		/* Global scoped. We need to tell the run-time that we're
		 * entering a global expression. */
		tb.tb_lastsym = NULL;
		tb.tb_type = t;
		tb.tb_symtab = symtabByName(t->getName());
		tb.tb_diskoff = rt_glue.getDynOffset(t);
		tb.tb_parambuf = rt_glue.getDynParams(t);

		found_expr = resolveTail(tb, ids, ++(ids->begin()));
		if (found_expr) {
			parent_closure = rt_glue.getDynClosure(t);
		}
	}

	/* nothing found, return error. */
	if (!found_expr) return NULL;

	/* massage the result into something we can use-- disk read or typepass */

	/* return typepass if we are returning a user type */
	thunk_field = tb.tb_lastsym->getFieldThunk();
	if (thunk_field->getType() != NULL)
		return FCall::mkClosure(
			tb.tb_diskoff,
			Expr::rewriteReplace(
				tb.tb_parambuf,
				rt_glue.getThunkArgIdx(),
				new Number(0)));

	/* not returning a user type-- we know that the size to 
	 * read is going to be constant. don't need to bother with computing
	 * parameters for the size function-- inline it! */
	delete tb.tb_parambuf;

	ret = rt_glue.getLocal(
		parent_closure,
		tb.tb_diskoff,
		thunk_field->getSize()->copyConstValue());
	return ret;
}

/** convert an id into a constant or convert to 
 *  access call or dynamic call */
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

			tf = st_ent->getFieldThunk();
			if (tf->getType() != NULL) {
				return FCall::mkClosure(
					tf->getOffset()->copyFCall(),
					Expr::rewriteReplace(
						tf->getParams()->copyFCall(),
						rt_glue.getThunkArgIdx(),
						new Number(0)));
			}

			offset = tf->getOffset()->copyFCall();
			return rt_glue.getLocal(
				rt_glue.getThunkClosure(),
				offset,
				tf->getSize()->copyFCall());
		}
	} else if (cur_func != NULL) {
		if (cur_func->getArgs()->hasField(id->getName())) {
			/* pass-through */
			return id->copy();
		}
	}

	/* support for access of dynamic types.. gets base bits for type */
	if ((t = typeByName(id->getName())) != NULL) {
		return FCall::mkClosure(
			rt_glue.getDynOffset(t),
			rt_glue.getDynParams(t));
	}

	/* could not resolve */
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
		rt_glue.getThunkClosure(),
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

	it = symtabs.find(s);
	if (it == symtabs.end())
		return NULL;

	return (*it).second;
}

const Type* EvalCtx::typeByName(const std::string& s) const
{
	const SymbolTable	*st;
	st = symtabByName(s);
	return (st != NULL) ? st->getOwnerType() : NULL;
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

	if (cur_func != NULL) {
		const Type*	ret;
		string		type_name;

		/* find inside function arguments */
		if (cur_func->getArgs()->lookupType(id->getName(), type_name))
			return types_map[type_name];

		/* find inside scope blocks */
		assert (cur_func_blk != NULL);
		if ((ret = cur_func_blk->getVarType(id->getName())) != NULL)
			return ret;
	}

	if (types_map.count(id->getName()) != 0)
		return types_map[id->getName()];

	return NULL;
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
		const Type*	t;

		f = funcs_map[fc->getName()];
		if (f == NULL)
			return NULL;

		t = types_map[f->getRet()];
		return t;
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


	/* arith expression? */
	cout << "WTF????????" << endl;
	return NULL;
}

