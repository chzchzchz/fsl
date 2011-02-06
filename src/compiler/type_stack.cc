#include "type_stack.h"
#include "runtime_interface.h"
#include "eval.h"

extern RTInterface	rt_glue;

TypeBase::TypeBase(
	const Type* in_tb_type,
	const SymbolTable* in_tb_symtab,
	const SymbolTableEnt* in_tb_lastsym,
	Expr* in_tb_diskoff,
	Expr* in_tb_parambuf,
	Expr* in_tb_virt)
:tb_type(in_tb_type), tb_symtab(in_tb_symtab), tb_lastsym(in_tb_lastsym),
 tb_diskoff(in_tb_diskoff), tb_parambuf(in_tb_parambuf), tb_virt(in_tb_virt)
{ }

TypeBase::~TypeBase()
{
	delete tb_diskoff;
	delete tb_parambuf;
	delete tb_virt;
}

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
		ret,
		rt_glue.getThunkClosure(),
		FCall::mkClosure(
			tb_diskoff->simplify(),
			tb_parambuf->simplify(),
			tb_virt->simplify()));

	/* XXX is this correct? */
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
		Expr	*array_elem_base;
		bool	is_union;

		*new_params = Expr::rewriteReplace(
			*new_params, new Id("@"), idx->simplify());
		is_union = t->isUnion();
		if (is_union == true || tf->getElems()->isFixed()) {
			Expr	*sz;

			sz = new AOPMul(
				replaceClosure(tf->getSize()->copyFCall()),
				evalReplace(*ectx, idx->simplify()));
			array_elem_base = new AOPAdd(new_base, sz);
		} else {
			array_elem_base = rt_glue.computeArrayBits(
				parent_type,
				tf->getFieldNum(),
				tb_diskoff,
				tb_parambuf,
				tb_virt,
				evalReplace(*ectx, idx->simplify()));
			array_elem_base = new AOPAdd(
				new_base, array_elem_base);
		}

		new_base = array_elem_base;
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

	assert (tb_lastsym != NULL);
	assert (tb_diskoff != NULL);
	assert (tb_parambuf != NULL);

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

	delete tb_diskoff;
	delete tb_parambuf;

	tb_diskoff = new_base;
	tb_parambuf = new_params;

	return true;

err_cleanup:
	if (new_base != NULL) delete new_base;
	if (new_params != NULL) delete new_params;

	return false;
}

bool TypeStack::followIdIdx(const EvalCtx* ectx, std::string& name, Expr* idx)
{
	TypeBase		*new_tb;
	const TypeBase		*top_tb;
	const Type		*tb_type;
	const SymbolTableEnt	*tb_sym;
	const SymbolTable	*tb_st;

	top_tb = getTop();
	tb_sym = top_tb->getSymTab()->lookup(name);
	if (tb_sym == NULL) return false;

	tb_type = tb_sym->getType();
	tb_st = (tb_type != NULL) ? symtabByName(tb_type->getName()):NULL;

	new_tb = new TypeBase(
		tb_type,
		tb_st,
		tb_sym,
		top_tb->getDiskOff()->copy(),
		top_tb->getParamBuf()->copy(),
		top_tb->getVirt()->copy());

	if (new_tb->setNewOffsets(ectx, getTop()->getType(), idx) == false) {
		delete new_tb;
		return false;
	}

	s.push(new_tb);
	return true;
}
