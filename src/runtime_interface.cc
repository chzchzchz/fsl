#include "runtime_interface.h"
#include "util.h"

Expr* RTInterface::getLocal(Expr* disk_bit_offset, Expr* num_bits)
{
	ExprList*	exprs = new ExprList();

	assert (disk_bit_offset != NULL);
	assert (num_bits != NULL);
	
	exprs->add(disk_bit_offset);
	exprs->add(num_bits);

	return new FCall(new Id("__getLocal"), exprs);
}

Expr* RTInterface::getLocalArray(
	Expr* idx, Expr* bits_in_type, Expr* base_offset, Expr* bits_in_array)
{
	ExprList	*exprs = new ExprList();

	assert (idx != NULL);
	assert (bits_in_type != NULL);
	assert (base_offset != NULL);
	assert (bits_in_array != NULL);

	exprs->add(idx);
	exprs->add(bits_in_type);
	exprs->add(base_offset);
	exprs->add(bits_in_array);

	return new FCall(new Id("__getLocalArray"), exprs);
}

Expr* RTInterface::getDyn(Expr* typenum)
{
	ExprList	*exprs = new ExprList();

	assert (typenum != NULL);

	exprs->add(typenum);

	return new FCall(new Id("__getDyn"), exprs);
}


Expr* RTInterface::getThunkArg(void)
{
	return new Id("__thunk_arg_off");
}

const std::string RTInterface::getThunkArgName(void)
{
	return "__thunk_arg_off";
}


Expr* RTInterface::maxValue(ExprList* exprs)
{
	assert (exprs != NULL);
	return new FCall(
		new Id("__max" + int_to_string(exprs->size())),
		exprs);
}

Expr* RTInterface::fail(void)
{
	return new FCall(new Id("fsl_fail"), new ExprList());
}
