#include <iostream>

#include "thunk_type.h"
#include "thunk_fieldsize.h"
#include "code_builder.h"
#include "symtab.h"
#include "runtime_interface.h"

using namespace std;

extern symtab_map	symtabs;
extern CodeBuilder*	code_builder;
extern RTInterface	rt_glue;

Expr* ThunkFieldSize::copyConstValue(void) const
{
	assert (raw_expr != NULL);
	assert (t == NULL);
	assert (dynamic_cast<const Number*>(raw_expr) != NULL);

	return raw_expr->copy();
}

ThunkFieldSize* ThunkFieldSize::copy(void) const
{
	ThunkFieldSize	*tfs;

	if (t != NULL)
		tfs = new ThunkFieldSize(t);
	else
		tfs = new ThunkFieldSize(raw_expr->copy());

	tfs->setFieldName(getFieldName());
	tfs->setOwner(getOwner());

	return tfs;
}

bool ThunkFieldSize::genCode(void) const
{
	if (t == NULL) {
		/* constant value, easy */
		code_builder->genCode(
			getOwner()->getType(), getFCallName(), raw_expr);
	} else {
		/* passed parent offset, needs field offset to 
		 * use type's sizeof operator .. */
		const SymbolTable	*st, *st_owner;
		const SymbolTableEnt	*st_ent;
		const ThunkType		*tt;
		const ThunkFieldOffset	*tf_off;
		const ThunkParams	*tf_params;
		Expr			*gen_expr;

		st = symtabs[t->getName()];
		st_owner = symtabs[getOwner()->getType()->getName()];
		assert (st != NULL);
		assert (st_owner != NULL);

		tt = st->getThunkType();
		gen_expr = tt->getSize()->copyFCall();

		st_ent = st_owner->lookup(getFieldName());
		assert (st_ent != NULL);
		tf_off = st_ent->getFieldThunk()->getOffset();
		tf_params = st_ent->getFieldThunk()->getParams();

		/* replace getSize fcall's thunk_offset with
		 * our offset. */
		gen_expr = Expr::rewriteReplace(
				gen_expr,
				rt_glue.getThunkClosure(),
				FCall::mkClosure(
					tf_off->copyFCall(),
					tf_params->copyFCall()));
		code_builder->genCode(
			getOwner()->getType(),
			getFCallName(), 
			gen_expr);

		delete gen_expr;
	}

	return true;

}

