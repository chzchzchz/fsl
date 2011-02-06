#include "eval.h"
#include "func.h"
#include "runtime_interface.h"
#include "code_builder.h"
#include "type_stack.h"
#include <string.h>

extern const Func	*gen_func;
extern func_map		funcs_map;
extern type_map		types_map;
extern symtab_map	symtabs;
extern const_map	constants;
extern CodeBuilder	*code_builder;
extern RTInterface	rt_glue;

using namespace std;

TypeStack* EvalCtx::resolveIdStructCurScope(
	const IdStruct* ids, const std::string& name, const Expr* idx) const
{
	TypeBase		*head;
	const Type		*head_type;
	const SymbolTable	*head_st;
	const SymbolTableEnt	*head_sym;

	if (cur_scope == NULL) return NULL;

	/* type scoped */
	/* reference to internal field (e.g. ino_num of ext2_inode) */
	head_sym = cur_scope->lookup(name);
	if (head_sym == NULL) return NULL;

	head_type = head_sym->getType();
	head_st = (head_type != NULL) ? symtabByName(head_type->getName()) : NULL;

	head = new TypeBase(
		head_type,
		head_st,
		head_sym,
		rt_glue.getThunkArgOffset(),
		rt_glue.getThunkArgParamPtr(),
		rt_glue.getThunkArgVirt());
	head->setNewOffsets(this, cur_scope->getOwnerType(), idx);

	return resolveTail(head, ids, ++(ids->begin()));
}

TypeStack* EvalCtx::resolveIdStructVarScope(
	const IdStruct* ids,
	const std::string& name,
	const VarScope* vs) const
{
	TypeBase		*head;
	const Type		*head_type;

	/* pull the typepass out of the vscoped arguments */
	if (vs == NULL) return NULL;

	head_type = vs->getVarType(name);
	if (head_type == NULL) return NULL;

	head = new TypeBase(
		head_type,
		symtabByName(head_type->getName()),
		NULL,
		new FCall(
			new Id("__extractOff"),
			new ExprList(new Id(name))),
		new FCall(
			new Id("__extractParam"),
			new ExprList(new Id(name))),
		new FCall(
			new Id("__extractVirt"),
			new ExprList(new Id(name))));

	return resolveTail(head, ids, ++(ids->begin()));
}

TypeStack* EvalCtx::resolveIdStructFunc(
	const IdStruct* ids, const std::string& name) const
{
	TypeBase		*head;
	const Type		*head_type;

	/* function scoped */
	/* pull the typepass out of the function arguments */
	assert  (cur_func_blk != NULL);

	head_type = cur_func->getArgs()->getType(name);
	if (head_type == NULL)
		head_type = cur_func_blk->getVarType(name);
	if (head_type == NULL) return NULL;

	head = new TypeBase(
		head_type,
		symtabByName(head_type->getName()),
		NULL,
		new FCall(new Id("__extractOff"), new ExprList(new Id(name))),
		new FCall(new Id("__extractParam"),
			new ExprList(new Id(name))),
		new FCall(new Id("__extractVirt"), new ExprList(new Id(name))));

	return resolveTail(head, ids, ++(ids->begin()));
}

TypeStack* EvalCtx::resolveIdStructFHead(
	const IdStruct* ids, Expr* &parent_closure) const
{
	const Type		*t;
	const FCall		*ids_f;
	const Func		*f;
	Expr			*evaled_ids_f;
	TypeBase		*head;
	const SymbolTable	*head_st;
	TypeStack		*ret_ts;

	ids_f = dynamic_cast<const FCall*>(ids->front());
	assert (ids_f != NULL);

	/* idstruct of form func(...).a.b.c... */
	if (funcs_map.count(ids_f->getName()) == 0) {
		cerr << "BAD FUNC NAME" << endl;
		return NULL;
	}

	f = funcs_map[ids_f->getName()];
	assert (f != NULL);

	t = types_map[f->getRet()];
	if (t == NULL) {
		cerr << "BAD RETURN TYPE" << endl;
		return NULL;
	}

	evaled_ids_f = eval(*this, ids_f);
	if (evaled_ids_f == NULL) {
		cerr << "OOPS. ";
		ids_f->print(cerr);
		cerr << endl;
		return NULL;
	}

	head_st = symtabByName(t->getName());
	head = new TypeBase(
		t,
		head_st,
		NULL,
		new FCall(
			new Id("__extractOff"),
			new ExprList(evaled_ids_f->copy())),
		new FCall(
			new Id("__extractParam"),
			new ExprList(evaled_ids_f->copy())),
		new FCall(
			new Id("__extractVirt"),
			new ExprList(evaled_ids_f->copy())));


	ret_ts = resolveTail(head, ids, ++(ids->begin()));
	if (ret_ts != NULL)  parent_closure = evaled_ids_f;
	return ret_ts;
}

TypeStack* EvalCtx::resolveTypeStack(
	const IdStruct* ids, Expr* &parent_closure) const
{
	TypeStack		*ts = NULL;
	const Type		*t;
	string			name;
	Expr			*idx;
	bool			found_expr;

	assert (ids != NULL);

	if (dynamic_cast<const FCall*>(ids->front()) != NULL)
		return resolveIdStructFHead(ids, parent_closure);

	if (toName(ids->front(), name, idx) == false) return false;

	found_expr = false;

	/* this will catch parameters passed to the type */
	if (cur_scope != NULL) {
		ts = resolveIdStructVarScope(
			ids, name, code_builder->getVarScope());
		if (ts) parent_closure = new Id(name);
	}

	/* this will catch parameters in any varscope */
	if (cur_vscope != NULL) {
		ts = resolveIdStructVarScope(ids, name, cur_vscope);
		if (ts) parent_closure = new Id(name);
	}

	if (ts == NULL) {
		ts = resolveIdStructCurScope(ids, name, idx);
		/* closure is from parent type.. */
		if (ts) parent_closure = rt_glue.getThunkClosure();
	}

	if (ts == NULL && idx != NULL) {
		/* idx on top level only permitted for expressions
		 * being resolved within the type scope */
		delete idx;
		cerr << "Idx on Func Arg." << endl;
		return NULL;
	}

	if (ts == NULL && cur_func_blk != NULL) {
		ts = resolveIdStructFunc(ids, name);
		/* closure is from head of idstruct */
		if (ts) parent_closure = new Id(name);
	}

	if (ts == NULL && (t = typeByName(name)) != NULL) {
		TypeBase	*head;

		assert (name == string("disk"));
		/* Global scoped. We need to tell the run-time that we're
		 * entering a global expression. */

		head = new TypeBase(
			t,
			symtabByName(t->getName()),
			NULL,
			new Number(0),
			new Id("__NULLPTR"),
			new Id("__NULLPTR8"));

		ts = resolveTail(head, ids, ++(ids->begin()));
		if (ts) parent_closure = FCall::mkBaseClosure(t);
	}

	delete idx;

	return ts;
}

Expr* EvalCtx::resolveVal(const IdStruct* ids) const
{
	const ThunkField	*thunk_field;
	TypeStack		*ts;
	const TypeBase		*top;
	Expr			*ret, *parent_closure = NULL;

	assert (ids != NULL);

	if ((ts = resolveTypeStack(ids, parent_closure)) == NULL) return NULL;

	/* massage the result into something we can use-- disk read or typepass */
	/* return typepass if we are returning a user type */
	top = ts->getTop();
	thunk_field = top->getSym()->getFieldThunk();
	if (thunk_field->getType() != NULL) {
		ret = FCall::mkClosure(
			top->getDiskOff()->copy(),
			Expr::rewriteReplace(
				top->getParamBuf()->copy(),
				rt_glue.getThunkArgIdx(),
				new Number(0)),
			top->getVirt()->copy());
		delete ts;
		return ret;
	}

	/* not returning a user type-- we know that the size to
	 * read is going to be constant. don't need to bother with computing
	 * parameters for the size function-- inline it! */
	assert (parent_closure != NULL);

	ret = rt_glue.getLocal(
		parent_closure,
		top->getDiskOff()->copy(),
		thunk_field->getSize()->copyConstValue());
	delete ts;
	return ret;
}

/** convert an id into a constant or convert to
 *  access call or dynamic call */
Expr* EvalCtx::resolveVal(const Id* id) const
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
		/* short-circuit, use current thunk (e.g. current type inst) */
		if (id->getName() == "this") return rt_glue.getThunkClosure();

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
						new Number(0)),
					rt_glue.getThunkArgVirt());
			}

			offset = tf->getOffset()->copyFCall();
			return rt_glue.getLocal(
				rt_glue.getThunkClosure(),
				offset,
				tf->getSize()->copyFCall());
		}
	} else if (cur_func != NULL) {
		/* pass-through name */
		if (cur_func->getArgs()->hasField(id->getName()))
			return id->copy();
	}

	/* NOTE: we need to change this for transparently nesting file systems
	 * 	Need to review some various tricks for base relocation
	 */
	if ((t = typeByName(id->getName())) != NULL) {
		assert (id->getName() == "disk");
		return FCall::mkBaseClosure(t);
	}

	/* could not resolve */
	return NULL;
}

Expr* EvalCtx::resolveArrayInType(const IdArray* ida) const
{
	/* array is in current scope */
	const SymbolTableEnt		*st_ent;
	Expr				*evaled_idx;
	const ThunkField		*tf;
	const Type			*t;

	assert (cur_scope != NULL);

	if ((st_ent = cur_scope->lookup(ida->getName())) == NULL) return NULL;

	evaled_idx = eval(*this, ida->getIdx());
	if (evaled_idx == NULL) {
		cerr << "Could not resolve ";
		(ida->getIdx())->print(cerr);
		cerr << endl;
		return NULL;
	}

	tf = st_ent->getFieldThunk();
	t = tf->getType();
	if (t != NULL) {
		/* best winter vacation ever */
		Expr	*params;
		Expr	*array_offset;
		bool	is_union;

		params = Expr::rewriteReplace(
			tf->getParams()->copyFCall(),
			new Id("@"),
			evaled_idx->simplify());
		params = Expr::rewriteReplace(
			params, rt_glue.getThunkArgIdx(), evaled_idx->simplify());
		array_offset = tf->getOffset()->copyFCall();

		is_union = t->isUnion();
		if (is_union == true || tf->getElems()->isFixed()) {
			Expr	*sz;

			sz = new AOPMul(
				tf->getSize()->copyFCall(),
				evaled_idx->simplify());
			array_offset = new AOPAdd(array_offset, sz);
		} else {
			Expr	*sz;
			sz = rt_glue.computeArrayBits(tf, evaled_idx);
			array_offset = new AOPAdd(sz, array_offset);
		}

		delete evaled_idx;
		return FCall::mkClosure(
			array_offset,
			params,
			rt_glue.getThunkArgVirt());
	}

	/* convert into __getLocalArray call */
	return rt_glue.getLocalArray(
		rt_glue.getThunkClosure(),
		evaled_idx,
		tf->getSize()->copyFCall(),
		tf->getOffset()->copyFCall(),
		new AOPMul(
			tf->getSize()->copyFCall(),
			tf->getElems()->copyFCall()));
}

Expr* EvalCtx::resolveVal(const IdArray* ida) const
{
	Expr				*ret;
	const_map::const_iterator	const_it;

	assert (ida != NULL);

	if (cur_scope != NULL) {
		ret = resolveArrayInType(ida);
		if (ret != NULL) return ret;
	}

/* TODO -- add support for constant arrays */
	assert (0 == 1 && "Could not resolve");

	/* could not resolve */
	return NULL;
}

/* get the physical disk location of the value in the idstruct */
Expr* EvalCtx::resolveLoc(const IdStruct* ids, Expr*& size) const
{
	Expr			*loc;
	Expr			*parent_closure = NULL;
	const ThunkField	*thunk_field;
	TypeStack		*ts;

	if ((ts = resolveTypeStack(ids, parent_closure)) == NULL) return NULL;

	cerr << "HI" << endl;
	ids->print(cerr);
	assert (parent_closure != NULL);
	loc = rt_glue.toPhys(parent_closure, ts->getTop()->getDiskOff());

	thunk_field = ts->getTop()->getSym()->getFieldThunk();
	size = thunk_field->getSize()->copyConstValue();

	delete ts;
	delete parent_closure;

	return loc;
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
		const Type*		our_type;
		const SymbolTableEnt*	st_ent;
		const ArgsList*		args;

		our_type = cur_scope->getOwnerType();
		if (id->getName() == "this") return our_type;

		st_ent = cur_scope->lookup(id->getName());
		if (st_ent != NULL) return types_map[st_ent->getTypeName()];

		/* might be a parameter to the type.. */
		args = our_type->getArgs();
		if (args != NULL) {
			const Type	*ret;
			ret = args->getType(id->getName());
			if (ret != NULL) return ret;
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

		if (cur_type == NULL) return NULL;

		cur_symtab = symtabByName(cur_type->getName());
		if (cur_symtab == NULL) return NULL;

		if ((id = dynamic_cast<const Id*>(*it)) != NULL) {
			fieldname = id->getName();
		} else if ((ida = dynamic_cast<const IdArray*>(*it)) != NULL) {
			fieldname = ida->getName();
		} else {
			/* id or ida expected */
			return NULL;
		}

		cur_st_ent = cur_symtab->lookup(fieldname);
		/* field not found in symtab? */
		if (cur_st_ent == NULL) return NULL;

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
		if (f == NULL) return NULL;

		t = types_map[f->getRet()];
		return t;
	}

	id = dynamic_cast<const Id*>(e);
	if (id != NULL) return getTypeId(id);

	ida = dynamic_cast<const IdArray*>(e);
	if (ida != NULL) {
		Id	tmp_id(ida->getName());
		return getTypeId(&tmp_id);
	}

	ids = dynamic_cast<const IdStruct*>(e);
	if (ids != NULL) return getTypeIdStruct(ids);

	/* arith expression? */
	cerr << "EvalCtx::getType(): WTF????????";
	e->print(cerr);
	cerr << endl;

	return NULL;
}

TypeStack* EvalCtx::resolveTail(
	TypeBase			*tb,	/* current base */
	const IdStruct			*ids,
	IdStruct::const_iterator	ids_begin) const
{
	TypeStack	*ts;
	string		name;
	Expr		*idx = NULL;

	assert (ids != NULL);

	ts = new TypeStack(tb);
	for (	IdStruct::const_iterator it = ids_begin;
		ids_begin != ids->end();
		ids_begin++)
	{
		if (ts->getTop()->getType() == NULL) break;
		if (toName(*ids_begin, name, idx) == false) goto cleanup_err;
		if (ts->followIdIdx(this, name, idx) == false) break;
	}

	return ts;

cleanup_err:
	delete ts;
	return NULL;
}
