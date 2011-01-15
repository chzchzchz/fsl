#include <vector>

#include "runtime_interface.h"
#include "thunk_field.h"
#include "type.h"
#include "util.h"
#include "code_builder.h"

using namespace std;

extern CodeBuilder* code_builder;

#define RTF_TYPE_VOID		0
#define RTF_TYPE_CLOSURE	1
#define RTF_TYPE_INT64		2
#define RTF_TYPE_INT64PTR	3
#define RTF_TYPE_INT8PTR	4

#define RTF_MAX_PARAMS		7

typedef unsigned int rtftype_t;
struct rt_func
{
	const char*	rtf_name;
	rtftype_t	rtf_ret;
	unsigned int	rtf_param_c;
	rtftype_t	rtf_params[RTF_MAX_PARAMS];
};

/* TODO: __max should be a proper vararg function */
struct rt_func rt_funcs[] =
{
	{	"__debugClosureOutcall", RTF_TYPE_VOID,
		2, {RTF_TYPE_INT64, RTF_TYPE_CLOSURE}},
	{	"__debugOutcall", RTF_TYPE_VOID, 1, {RTF_TYPE_INT64}},
	{ 	"__getLocal",
		RTF_TYPE_INT64,
		3,
		{ RTF_TYPE_CLOSURE, RTF_TYPE_INT64, RTF_TYPE_INT64}
	},
	{	"__getLocalArray",
		RTF_TYPE_INT64,
		5,
		{RTF_TYPE_CLOSURE, RTF_TYPE_INT64, RTF_TYPE_INT64,
		 RTF_TYPE_INT64, RTF_TYPE_INT64},
	},
	{"fsl_fail" , RTF_TYPE_INT64 /* fake it */, 1, {RTF_TYPE_INT64} },
	{ "__max2", RTF_TYPE_INT64,  2, {RTF_TYPE_INT64, RTF_TYPE_INT64} },
	{"__max3",
		RTF_TYPE_INT64,
		3,
		{RTF_TYPE_INT64, RTF_TYPE_INT64, RTF_TYPE_INT64}
	},
	{"__max4",
		 RTF_TYPE_INT64,
		4,
		{RTF_TYPE_INT64, RTF_TYPE_INT64, RTF_TYPE_INT64, RTF_TYPE_INT64}
	},
	{"__max5",
		 RTF_TYPE_INT64,
		5,
		{RTF_TYPE_INT64, RTF_TYPE_INT64, RTF_TYPE_INT64, RTF_TYPE_INT64,
		 RTF_TYPE_INT64}
	},
	{"__max6",
		 RTF_TYPE_INT64,
		6,
		{RTF_TYPE_INT64, RTF_TYPE_INT64, RTF_TYPE_INT64, RTF_TYPE_INT64,
		RTF_TYPE_INT64,RTF_TYPE_INT64}
	},
	{"__max7",
		 RTF_TYPE_INT64,
		7,
		{RTF_TYPE_INT64, RTF_TYPE_INT64, RTF_TYPE_INT64, RTF_TYPE_INT64,
		 RTF_TYPE_INT64, RTF_TYPE_INT64, RTF_TYPE_INT64}
	},
	{"__computeArrayBits",
		 RTF_TYPE_INT64,
		4,
		{RTF_TYPE_INT64, /* parent typenum */
		 RTF_TYPE_CLOSURE, /* parent closure */
		 RTF_TYPE_INT64,  /* field idx into parent type */
		 RTF_TYPE_INT64 /* element number to grab */}
	},
	{"__writeVal", RTF_TYPE_VOID, 3, {RTF_TYPE_INT64, RTF_TYPE_INT64, RTF_TYPE_INT64}},
	{"__toPhys", RTF_TYPE_INT64, 2, {RTF_TYPE_CLOSURE, RTF_TYPE_INT64}},
	{ NULL }
};

static const llvm::Type* rtf_type2llvm(rtftype_t t)
{
	llvm::LLVMContext	&gctx(llvm::getGlobalContext());

	switch (t) {
	case RTF_TYPE_VOID:	return llvm::Type::getVoidTy(gctx);
	case RTF_TYPE_CLOSURE:	return code_builder->getClosureTyPtr();
	case RTF_TYPE_INT64:	return llvm::Type::getInt64Ty(gctx);
	case RTF_TYPE_INT64PTR:	return llvm::Type::getInt64PtrTy(gctx);
	case RTF_TYPE_INT8PTR:	return llvm::Type::getInt8PtrTy(gctx);
	}

	assert (0 == 1 && "Undefiend llvm type");
	return NULL;
}
static void rt_func_args(
	const struct rt_func* rtf,
	vector<const llvm::Type*>& args)
{
	args.clear();
	for (unsigned int i = 0; i < rtf->rtf_param_c; i++) {
		args.push_back(rtf_type2llvm(rtf->rtf_params[i]));
	}
}

/**
 * insert run-time functions into the llvm module so that they resolve
 * when called..
 */
void RTInterface::loadRunTimeFuncs(CodeBuilder* cb)
{
	vector<const llvm::Type*>	args;
	llvm::FunctionType		*ft;
	llvm::Function			*f;

	for (unsigned int k = 0; rt_funcs[k].rtf_name != NULL; k++) {
		assert (rt_funcs[k].rtf_param_c <= RTF_MAX_PARAMS &&
			"Too many params for func. Bump RTF_MAX_PARAMS.");
		rt_func_args(&rt_funcs[k], args);
		ft = llvm::FunctionType::get(
			rtf_type2llvm(rt_funcs[k].rtf_ret),
			args,
			false);
	
		f = llvm::Function::Create(
			ft,
			llvm::Function::ExternalLinkage, 
			rt_funcs[k].rtf_name,
			cb->getModule());
	}
}

Expr* RTInterface::getDebugCall(Expr* pass_val)
{
	return new FCall(
		new Id("__debugOutcall"),
		new ExprList(pass_val));
}

Expr* RTInterface::getDebugCallTypeInstance(
	const Type* clo_type, Expr* clo_expr)
{
	return new FCall(
		new Id("__debugClosureOutcall"),
		new ExprList(new Number(clo_type->getTypeNum()), clo_expr));

}

Expr* RTInterface::getLocal(Expr* closure, Expr* disk_bit_offset, Expr* num_bits)
{
	ExprList	*exprs = new ExprList();

	assert (disk_bit_offset != NULL);
	assert (num_bits != NULL);

	exprs->add(closure);
	exprs->add(disk_bit_offset);
	exprs->add(num_bits);
	
	return new FCall(new Id("__getLocal"), exprs);
}

Expr* RTInterface::getLocalArray(
	Expr* clo,
	Expr* idx, Expr* bits_in_type, Expr* base_offset, Expr* bits_in_array)
{
	ExprList	*exprs = new ExprList();

	assert (clo != NULL);
	assert (idx != NULL);
	assert (bits_in_type != NULL);
	assert (base_offset != NULL);
	assert (bits_in_array != NULL);

	exprs->add(clo);
	exprs->add(idx);
	exprs->add(bits_in_type);
	exprs->add(base_offset);
	exprs->add(bits_in_array);

	return new FCall(new Id("__getLocalArray"), exprs);
}


Expr* RTInterface::getThunkClosure(void)
{
	return new Id(getThunkClosureName());
}

Expr* RTInterface::getThunkArgOffset(void)
{
	return new Id(getThunkArgOffsetName());
}

Expr* RTInterface::getThunkArgParamPtr(void)
{
	return new Id(getThunkArgParamPtrName());
}

Expr* RTInterface::getThunkArgVirt(void)
{
	return new Id(getThunkArgVirtName());
}

Expr* RTInterface::getThunkArgIdx(void)
{
	return new Id(getThunkArgIdxName());
}

const std::string RTInterface::getThunkArgIdxName(void)
{
	return "__thunkparamsf_idx";
}

const std::string RTInterface::getThunkClosureName(void)
{
	return "__thunk_closure";
}

const std::string RTInterface::getThunkArgVirtName(void)
{
	return "__thunk_virt";
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
	return new FCall(new Id("fsl_fail"), new ExprList(new Number(0)));
}

Expr* RTInterface::computeArrayBits(const ThunkField* tf)
{
	Expr	*end_idx, *ret;
	end_idx = tf->getElems()->copyFCall();
	ret = computeArrayBits(tf, end_idx);
	delete end_idx;
	return ret;
}

/* compute number of bits in a given array */
Expr* RTInterface::computeArrayBits(const ThunkField* tf, const Expr* idx)
{
	ExprList	*exprs;
	const Type	*owner_type;
	const Type	*t;

	assert (tf != NULL);

	t = tf->getType();
	owner_type = tf->getOwnerType();

	assert (t != NULL);
	assert (t->isUnion() == false);
	assert (tf->getElems()->isSingleton() == false);
	assert (owner_type != NULL);

	exprs = new ExprList();
	/* parent type num */
	exprs->add(new Number(owner_type->getTypeNum()));
	/* parent closure */
	exprs->add(getThunkClosure());
	/* field idx */
	exprs->add(new Number(tf->getFieldNum()));
	/* num elements */
	exprs->add(idx->copy());

	return new FCall(new Id("__computeArrayBits"), exprs);
}

Expr* RTInterface::computeArrayBits(
	/* type of the parent*/
	const Type* t,
	/* fieldall idx for type */
	unsigned int fieldall_num,
	/* converted into single closure */
	const Expr* diskoff, 	/* base */
	const Expr* params,	/* params */
	const Expr* virt,	/* virt */
	/* nth element to find */
	const Expr* num_elems)
{
	ExprList	*exprs;

	assert (t != NULL);
	assert (diskoff != NULL && params != NULL);

	exprs = new ExprList();
	exprs->add(new Number(t->getTypeNum()));
	exprs->add(FCall::mkClosure(
		diskoff->copy(), params->copy(), virt->copy()));
	exprs->add(new Number(fieldall_num));
	exprs->add(num_elems->copy());

	return new FCall(new Id("__computeArrayBits"), exprs);
}

Expr* RTInterface::writeVal(const Expr* loc, const Expr* sz, const Expr* val)
{
	ExprList	*exprs = new ExprList();

	exprs->add(loc->copy());
	exprs->add(sz->copy());
	exprs->add(val->copy());
	return new FCall(new Id("__writeVal"), exprs);
}

Expr* RTInterface::toPhys(const Expr* clo, const Expr* off)
{
	return new FCall(new Id("__toPhys"),
		new ExprList(clo->copy(), off->copy()));
}
