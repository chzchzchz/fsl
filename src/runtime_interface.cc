#include <vector>

#include "runtime_interface.h"
#include "thunk_field.h"
#include "type.h"
#include "util.h"
#include "code_builder.h"

using namespace std;

#define NUM_RUNTIME_FUNCS	12

/* TODO: __max should be a proper vararg function */
static const char*	rt_func_names[] = {	
	"__getLocal", "__getLocalArray", "__getDynOffset", "fsl_fail", 
	"__max2", "__max3", "__max4", "__max5",
	"__max6", "__max7",
	"__enterDynCall", "__leaveDynCall",
};

static int rt_func_arg_c[] = {
	2,4,1,0, 
	2,3,4,5,
	6, 7,
	0, 0};

/**
 * insert run-time functions into the llvm module so that they resolve
 * when called..
 */
void RTInterface::loadRunTimeFuncs(CodeBuilder* cb)
{
	vector<const llvm::Type*>	args;
	llvm::FunctionType		*ft;
	llvm::Function			*f;
	llvm::LLVMContext		&gctx(llvm::getGlobalContext());


	for (int i = 0; i < NUM_RUNTIME_FUNCS; i++) {
		args = vector<const llvm::Type*>(
			rt_func_arg_c[i], 
			llvm::Type::getInt64Ty(gctx));

		ft = llvm::FunctionType::get(
			llvm::Type::getInt64Ty(gctx),
			args,
			false);
	
		f = llvm::Function::Create(
			ft,
			llvm::Function::ExternalLinkage, 
			rt_func_names[i],
			cb->getModule());
	}

	/* add getDynParams -- it has a specila form */
	args.clear();
	args.push_back(llvm::Type::getInt64Ty(gctx));
	args.push_back(llvm::Type::getInt64PtrTy(gctx));
	ft = llvm::FunctionType::get(
		llvm::Type::getVoidTy(gctx),
		args,
		false);
	
	llvm::Function::Create(ft,
		llvm::Function::ExternalLinkage,
		"__getDynParams", 
		cb->getModule());

	args.clear();
	args.push_back(llvm::Type::getInt64Ty(gctx));
	args.push_back(llvm::Type::getInt64Ty(gctx));
	args.push_back(llvm::Type::getInt64PtrTy(gctx));
	args.push_back(llvm::Type::getInt64Ty(gctx));
	ft = llvm::FunctionType::get(
		llvm::Type::getInt64Ty(gctx),
		args,
		false);
	llvm::Function::Create(ft,
		llvm::Function::ExternalLinkage,
		"__computeArrayBits", 
		cb->getModule());

}


Expr* RTInterface::getLocal(Expr* disk_bit_offset, Expr* num_bits)
{
	assert (disk_bit_offset != NULL);
	assert (num_bits != NULL);
	
	return new FCall(
		new Id("__getLocal"), 
		new ExprList(disk_bit_offset, num_bits));
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

Expr* RTInterface::getEnterDynCall(void) const
{
	return new FCall(new Id("__enterDynCall"), new ExprList());
}

Expr* RTInterface::getLeaveDynCall(void) const
{
	return new FCall(new Id("__leaveDynCall"), new ExprList());
}


Expr* RTInterface::getDynOffset(const Type* user_type)
{
	assert (user_type != NULL);

	return new FCall(
		new Id("__getDynOffset"), 
		new ExprList(new Number(user_type->getTypeNum())));
}

Expr* RTInterface::getDynParams(const Type* user_type)
{
	return new FCall(
		new Id("__getDynParams_preAlloca"), 
		new ExprList(new Number(user_type->getTypeNum())));
}


Expr* RTInterface::getThunkArgOffset(void)
{
	return new Id(getThunkArgOffsetName());
}

Expr*	RTInterface::getThunkArgParamPtr(void)
{
	return new Id(getThunkArgParamPtrName());
}

const std::string RTInterface::getThunkArgOffsetName(void)
{
	return "__thunk_arg_off";
}

const std::string RTInterface::getThunkArgParamPtrName(void)
{
	return "__thunk_arg_params";
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
	const Type	*t;

	assert (tf != NULL);

	t = tf->getType();
	assert (t != NULL);
	assert (t->isUnion() == false);
	assert (tf->getElems()->isSingleton() == false);

	exprs = new ExprList();
	exprs->add(new Number(tf->getType()->getTypeNum()));
	exprs->add(tf->getOffset()->copyFCall());
	exprs->add(tf->getParams()->copyFCall());
	exprs->add(tf->getElems()->copyFCall());

	return new FCall(new Id("__computeArrayBits"), exprs);
}

Expr* RTInterface::computeArrayBits(
	const Type* t,
	const Expr* diskoff, const Expr* params,
	const Expr* idx)
{
	ExprList	*exprs;

	assert (t != NULL);
	assert (t->isUnion() == false);

	exprs = new ExprList();
	exprs->add(new Number(t->getTypeNum()));
	exprs->add(diskoff->copy());
	exprs->add(params->copy());
	exprs->add(idx->copy());

	return new FCall(new Id("__computeArrayBits"), exprs);
}

