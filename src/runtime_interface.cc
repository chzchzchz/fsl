#include <vector>

#include "runtime_interface.h"
#include "thunk_field.h"
#include "type.h"
#include "util.h"
#include "code_builder.h"

using namespace std;

#define NUM_RUNTIME_FUNCS	11

/* TODO: __max should be a proper vararg function */
static const char*	rt_func_names[] = {	
	"__getLocal", "__getLocalArray", "__getDyn", "fsl_fail", 
	"__max2", "__max3", "__max4", "__max5",
	"__max6", "__max7",
	"__computeArrayBits"};

static int rt_func_arg_c[] = {
	2,4,1,0, 
	2,3,4,5,
	6, 7,
	3};

/**
 * insert run-time functions into the llvm module so that they resolve
 * when called..
 */
void RTInterface::loadRunTimeFuncs(CodeBuilder* cb)
{
	for (int i = 0; i < NUM_RUNTIME_FUNCS; i++) {
		vector<const llvm::Type*>	args;
		llvm::FunctionType		*ft;
		llvm::Function			*f;

		args = vector<const llvm::Type*>(
			rt_func_arg_c[i], 
			llvm::Type::getInt64Ty(llvm::getGlobalContext()));

		ft = llvm::FunctionType::get(
			llvm::Type::getInt64Ty(llvm::getGlobalContext()),
			args,
			false);
	
		f = llvm::Function::Create(
			ft,
			llvm::Function::ExternalLinkage, 
			rt_func_names[i],
			cb->getModule());
	}
}

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

Expr* RTInterface::getDyn(const Type* user_type)
{
	ExprList	*exprs; 

	assert (user_type != NULL);

	exprs = new ExprList(new Number(user_type->getTypeNum()));
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

/* compute number of bits in a given array */
Expr* RTInterface::computeArrayBits(const ThunkField* tf)
{
	ExprList	*exprs;

	assert (tf->getElems()->isSingleton() == false);
	assert (tf->getType() != NULL);

	exprs = new ExprList();
	exprs->add(new Number(tf->getType()->getTypeNum()));
	exprs->add(tf->getOffset()->copyFCall());
	exprs->add(tf->getElems()->copyFCall());

	return new FCall(
		new Id("__computeArrayBits"),
		exprs);
}
