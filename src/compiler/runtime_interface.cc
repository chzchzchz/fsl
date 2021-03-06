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
#define RTF_FL_RO		0x80000000

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
		RTF_TYPE_INT64 | RTF_FL_RO,
		3,
		{ RTF_TYPE_CLOSURE, RTF_TYPE_INT64, RTF_TYPE_INT64}
	},
	{	"__getLocalArray",
		RTF_TYPE_INT64 | RTF_FL_RO,
		5,
		{RTF_TYPE_CLOSURE, RTF_TYPE_INT64, RTF_TYPE_INT64,
		 RTF_TYPE_INT64, RTF_TYPE_INT64},
	},
	{"fsl_fail" , RTF_TYPE_INT64 /* fake it */, 1, {RTF_TYPE_INT64} },
	{ "__max2", RTF_TYPE_INT64 | RTF_FL_RO,  2, {RTF_TYPE_INT64, RTF_TYPE_INT64} },
	{"__max3",
		RTF_TYPE_INT64 | RTF_FL_RO,
		3,
		{RTF_TYPE_INT64, RTF_TYPE_INT64, RTF_TYPE_INT64}
	},
	{"__max4",
		 RTF_TYPE_INT64 | RTF_FL_RO,
		4,
		{RTF_TYPE_INT64, RTF_TYPE_INT64, RTF_TYPE_INT64, RTF_TYPE_INT64}
	},
	{"__max5",
		 RTF_TYPE_INT64 | RTF_FL_RO,
		5,
		{RTF_TYPE_INT64, RTF_TYPE_INT64, RTF_TYPE_INT64, RTF_TYPE_INT64,
		 RTF_TYPE_INT64}
	},
	{"__max6",
		 RTF_TYPE_INT64 | RTF_FL_RO,
		6,
		{RTF_TYPE_INT64, RTF_TYPE_INT64, RTF_TYPE_INT64, RTF_TYPE_INT64,
		RTF_TYPE_INT64,RTF_TYPE_INT64}
	},
	{"__max7",
		 RTF_TYPE_INT64 | RTF_FL_RO,
		7,
		{RTF_TYPE_INT64, RTF_TYPE_INT64, RTF_TYPE_INT64, RTF_TYPE_INT64,
		 RTF_TYPE_INT64, RTF_TYPE_INT64, RTF_TYPE_INT64}
	},
	{"__computeArrayBits",
		 RTF_TYPE_INT64 | RTF_FL_RO,
		4,
		{RTF_TYPE_INT64, /* parent typenum */
		 RTF_TYPE_CLOSURE, /* parent closure */
		 RTF_TYPE_INT64,  /* field idx into parent type */
		 RTF_TYPE_INT64 /* element number to grab */}
	},
	{"__writeVal", RTF_TYPE_VOID, 3, {RTF_TYPE_INT64, RTF_TYPE_INT64, RTF_TYPE_INT64}},
	{"__toPhys", RTF_TYPE_INT64 | RTF_FL_RO, 2, {RTF_TYPE_CLOSURE, RTF_TYPE_INT64}},
	{ NULL }
};

static llvm::Type* rtf_type2llvm(rtftype_t t)
{
	llvm::LLVMContext	&gctx(llvm::getGlobalContext());

	switch (t & 0xf) {
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
	vector<llvm::Type*>& args)
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
	for (unsigned int k = 0; rt_funcs[k].rtf_name != NULL; k++) {
		vector<llvm::Type*>	args;
		rtftype_t			rtft;
		llvm::Function			*f;

		assert (rt_funcs[k].rtf_param_c <= RTF_MAX_PARAMS &&
			"Too many params for func. Bump RTF_MAX_PARAMS.");
		rt_func_args(&rt_funcs[k], args);
		rtft = rt_funcs[k].rtf_ret;
		f = code_builder->genProtoV(
			rt_funcs[k].rtf_name,
			rtf_type2llvm(rtft),
			args);
		if (rtft & RTF_FL_RO) f->setOnlyReadsMemory();
	/* breaks function unwinding in gdb, only use on release */
#ifdef FSL_RELEASE
		f->setDoesNotThrow();
#endif
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

	/* parent type num, parent clo, field idx, num elems */
	exprs = new ExprList();
	exprs->add(new Number(owner_type->getTypeNum()));
	exprs->add(getThunkClosure());
	exprs->add(new Number(tf->getFieldNum()));
	exprs->add(idx->copy());

	return new FCall(new Id("__computeArrayBits"), exprs);
}

Expr* RTInterface::computeArrayBits(
	/* type of the parent*/
	const Type* t,
	/* fieldall idx for type */
	unsigned int fieldall_num,
	/* converted into single closure */
	const Expr* clo,
	/* nth element to find */
	const Expr* num_elems)
{
	ExprList	*exprs;

	assert (t != NULL);

	exprs = new ExprList();
	exprs->add(new Number(t->getTypeNum()));
	exprs->add(clo->copy());
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

Expr* RTInterface::toPhys(Expr* clo, Expr* off)
{
	return new FCall(new Id("__toPhys"), new ExprList(clo, off));
}
