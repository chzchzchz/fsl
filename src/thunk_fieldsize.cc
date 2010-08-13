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

const std::string ThunkFieldSize::getFCallName(void) const
{
	/* TODO: support field sizes that are type size aliases */

	return "__thunkfieldsize_"+
		owner->getType()->getName() + "_" +
		fieldname;
}


FCall* ThunkFieldSize::copyFCall(void) const
{
	return new FCall(
		new Id(getFCallName()),
		owner->copyExprList());
}

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
		tfs = new ThunkFieldSize(owner, fieldname, t);
	else
		tfs = new ThunkFieldSize(
			owner, fieldname, raw_expr->copy());

	tfs->tf_owner = tf_owner;

	return tfs;
}

bool ThunkFieldSize::genCode(void) const
{
	if (t == NULL) {
		code_builder->genCode(
			owner->getType(), getFCallName(), raw_expr);
	} else {
		const ThunkType		*tt;
		const ThunkFieldOffset	*tf_off;
		Expr			*gen_expr;

		assert (tf_owner != NULL);

		tt = symtabs[t->getName()]->getThunkType();
		gen_expr = tt->getSize()->copyFCall();
		tf_off = tf_owner->getOffset();

		/* replace getSize fcall's thunk_offset with
		 * our offset. */
		gen_expr = Expr::rewriteReplace(
				gen_expr,
				rt_glue.getThunkArg(),
				tf_off->copyFCall());
		code_builder->genCode(
			owner->getType(),
			getFCallName(), 
			gen_expr);

		delete gen_expr;
	}

	return true;

}

