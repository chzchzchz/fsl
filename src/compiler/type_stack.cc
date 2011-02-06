#include "type_stack.h"
#include "runtime_interface.h"
#include "eval.h"
#include <iostream>

using namespace std;
extern RTInterface	rt_glue;
extern type_map		types_map;

TypeBase::TypeBase(
	const Type* in_tb_type,
	const SymbolTable* in_tb_symtab,
	const SymbolTableEnt* in_tb_lastsym,
	Expr* in_tb_clo,
	bool in_is_value)
:tb_type(in_tb_type), tb_symtab(in_tb_symtab), tb_lastsym(in_tb_lastsym),
 tb_clo(in_tb_clo), is_value(in_is_value)
{ assert (tb_clo != NULL); }

TypeBase::~TypeBase() { delete tb_clo; }

TypeStack::TypeStack(TypeBase* tb)
{
	assert (tb != NULL);
	s.push(tb);
}

TypeStack::~TypeStack()
{
	while (!s.empty()) {
		delete s.top();
		s.pop();
	}
}

Expr* TypeBase::replaceClosure(Expr* in_expr) const
{
	Expr	*ret = in_expr;

	ret = Expr::rewriteReplace(
		ret, rt_glue.getThunkClosure(), tb_clo->copy());
	/* XXX is this correct?-- it's a rebase.. */
	ret = Expr::rewriteReplace(
		ret, rt_glue.getThunkArgIdx(), new Number(0));

	return ret;
}

/* follows an index in an array */
Expr* TypeBase::setNewOffsetsArray(
	const EvalCtx* ectx,
	const Type* parent_type,
	Expr* new_base,
	Expr** new_params,
	const ThunkField* tf,
	const Expr* idx) const
{
	const Type	*t;

	assert (new_params != NULL && *new_params != NULL);

	/* no index? => scalar => error */
	if (idx == NULL) return NULL;

	t = tf->getType();
	if (t != NULL) {
		/* non-constant type size-- call out to runtime */
		Expr	*elem_base;	/* array elem base */
		bool	is_union;

		*new_params = Expr::rewriteReplace(
			*new_params, new Id("@"), idx->simplify());
		is_union = t->isUnion();
		if (is_union == true || tf->getElems()->isFixed()) {
			Expr	*sz;

			sz = new AOPMul(
				replaceClosure(tf->getSize()->copyFCall()),
				evalReplace(*ectx, idx->simplify()));
			elem_base = new AOPAdd(new_base, sz);
		} else {
			elem_base = rt_glue.computeArrayBits(
				parent_type,
				tf->getFieldNum(),
				tb_clo,
				evalReplace(*ectx, idx->simplify()));
			elem_base = new AOPAdd(new_base, elem_base);
		}

		new_base = elem_base;
	} else {
		/* constant type size-- just multiply */
		Expr	*field_size;
		Expr	*array_off;

		field_size = replaceClosure(tf->getSize()->copyFCall());
		array_off = evalReplace(*ectx, idx->simplify());

		new_base = new AOPAdd(
			new_base, new AOPMul(field_size, array_off));
	}

	return new_base;
}

Expr* TypeBase::getDiskOff(void) const
{
	return FCall::extractCloOff(tb_clo);
}

Expr* TypeBase::getParamBuf(void) const
{
	return FCall::extractCloParam(tb_clo);
}

Expr* TypeBase::getVirt(void) const
{
	return FCall::extractCloVirt(tb_clo);
}

/* follows the last sym found for a type base */
bool TypeBase::setNewOffsets(
	const EvalCtx* ectx,
	const Type* parent_type,
	const Expr* idx)
{
	const ThunkField	*tf;
	Expr			*new_base = NULL;
	Expr			*new_params = NULL;
	Expr			*offset_fc;
	Expr			*new_clo;

	assert (tb_lastsym != NULL);

	tf = tb_lastsym->getFieldThunk();
	assert (tf != NULL);

	/* compute base and params for base element */
	/* if this isn't an array, we're already done! */
	offset_fc = tf->getOffset()->copyFCall();
	assert (offset_fc != NULL);

	new_base = replaceClosure(offset_fc);
	new_params = replaceClosure(tf->getParams()->copyFCall());

	if (tf->getElems()->isSingleton() == false) {
		new_base = setNewOffsetsArray(
			ectx, parent_type, new_base, &new_params, tf, idx);
		if (new_base == NULL) goto err_cleanup;
	} else {
		/* trying to index into a scalar type? */
		if (idx != NULL) goto err_cleanup;

		/* otherwise, we already have the offset+param we need... */
	}

	new_clo = FCall::mkClosure(new_base, new_params, getVirt());
	delete tb_clo;
	tb_clo = new_clo;

	return true;

err_cleanup:
	if (new_base != NULL) delete new_base;
	if (new_params != NULL) delete new_params;

	return false;
}

bool TypeStack::followIdIdx(const EvalCtx* ectx, std::string& name, Expr* idx)
{
	const TypeBase		*top_tb;
	TypeBase		*new_tb;
	const Type		*tb_type, *new_tb_type;
	const SymbolTableEnt	*new_tb_sym;
	const SymbolTable	*new_tb_st;
	Expr			*clo;
	bool			is_val;

	top_tb = getTop();
	tb_type = top_tb->getType();
	new_tb_sym = top_tb->getSymTab()->lookup(name);
	new_tb_st = NULL;
	if (new_tb_sym == NULL) {
		/* check if it's an arg.. */
		const ArgsList*	args;
		int		arg_idx, pb_idx;
		string		type_ret;

		assert (tb_type != NULL && "Arg needs a type");
		args = tb_type->getArgs();
		if (args == NULL || args->lookupType(name, type_ret) == false) {
			cerr << name << ": ARG NOT FOUND??" << endl;
			return false;
		}

		arg_idx = args->find(name);
		pb_idx = args->getParamBufBaseIdx(arg_idx);

		if (types_map.count(type_ret) == 0) {
			/* pull data from parambuf as value */
			Expr	*pb;
			pb = top_tb->getParamBuf();
			clo = FCall::extractParamVal(pb, new Number(pb_idx));
			delete pb;
			new_tb_type = NULL;
		} else {
			/* make closure from parambuf */
			new_tb_type = types_map[type_ret];
			assert (0 == 1);
		}
		is_val = true;
	} else {
		new_tb_type = new_tb_sym->getType();
		if (new_tb_type != NULL)
			new_tb_st = symtabByName(new_tb_type->getName());
		clo = top_tb->getClosureExpr()->copy();
		is_val = false;
	}

	new_tb = new TypeBase(new_tb_type, new_tb_st, new_tb_sym, clo, is_val);
	assert (new_tb_sym == new_tb->getSym());

	if (	!is_val &&
		new_tb->setNewOffsets(ectx, getTop()->getType(), idx) == false)
	{
		delete new_tb;
		return false;
	}

	s.push(new_tb);

	return true;
}
