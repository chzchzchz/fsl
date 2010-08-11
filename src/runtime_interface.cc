#include "runtime_interface.h"

Expr* RTInterface::getLocal(Expr* disk_bit_offset, Expr* num_bits)
{
	ExprList*	exprs = new ExprList();

	exprs->add(disk_bit_offset);
	exprs->add(num_bits);

	return new FCall(new Id("__getLocal"), exprs);
}

Expr* RTInterface::getLocalArray(
	Expr* idx, Expr* bits_in_type, Expr* base_offset, Expr* bits_in_array)
{
	ExprList	*exprs = new ExprList();

	exprs->add(idx);
	exprs->add(bits_in_type);
	exprs->add(base_offset);
	exprs->add(bits_in_array);

	return new FCall(new Id("__getLocalArray"), exprs);
}

Expr* RTInterface::getDyn(Expr* typenum)
{
	ExprList	*exprs = new ExprList();

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
