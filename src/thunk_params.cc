#include "thunk_params.h"
#include "code_builder.h"
#include "runtime_interface.h"

extern CodeBuilder*	code_builder;
extern RTInterface	rt_glue;

using namespace std;

static bool has_gen_empty_proto = false;
static bool has_gen_empty_code = false; 

FCall* ThunkParams::copyFCall(void) const
{
	ExprList	*exprs;

	exprs = new ExprList();
	exprs->add(rt_glue.getThunkArgOffset());
	exprs->add(rt_glue.getThunkArgParamPtr());
	exprs->add(new Id("__THUNKPARAMS_OUTPUT_PARAM"));

	return new FCall(new Id(getFCallName()), exprs);
}

ThunkParams* ThunkParams::copy(void) const
{
	ThunkParams	*ret;

	if (no_params) {
		ret = createNoParams();
	} else {
		assert (0 == 1);
	}

	ret->setFieldName(getFieldName());
	ret->setOwner(getOwner());

	return ret;
}

bool ThunkParams::genCodeEmpty(void)
{
	if (::has_gen_empty_code)
		return true;

	code_builder->genCodeEmpty(THUNKPARAM_EMPTYNAME);

	::has_gen_empty_code = true;

	return true;
}

bool ThunkParams::genProtoEmpty(void)
{
	if (::has_gen_empty_proto)
		return true;

	genProtoByName(THUNKPARAM_EMPTYNAME);

	::has_gen_empty_proto = true;

	return true;
}

bool ThunkParams::genProtoByName(const string& name)
{
	llvm::Function			*f;
	llvm::FunctionType		*ft;
	vector<const llvm::Type*>	args;

	/* diskoff */
	/* parent's params */
	/* out params */
	args.push_back(llvm::Type::getInt64Ty(llvm::getGlobalContext()));
	args.push_back(llvm::Type::getInt64PtrTy(llvm::getGlobalContext()));
	args.push_back(llvm::Type::getInt64PtrTy(llvm::getGlobalContext()));

	ft = llvm::FunctionType::get(
		llvm::Type::getVoidTy(llvm::getGlobalContext()),
		args,
		false);

	f = llvm::Function::Create(
		ft,
		llvm::Function::ExternalLinkage, 
		name,
		code_builder->getModule());

	return (f->getName() == name);
}


bool ThunkParams::genProto(void) const
{
	if (no_params) return genProtoEmpty();

	return genProtoByName(getFCallName());
}

/**
 * generate code that will do
 * parent = (diskoff_t, parambuf_t) -> (parambuf_t) = field
 * where we have f(parent, field)
 */
bool ThunkParams::genCode(void) const 
{
	if (no_params) return genCodeEmpty();

	assert (0 == 1);
}


ThunkParams* ThunkParams::createNoParams()
{
	/* used when there are no parameters assocaited with a field */
	return new ThunkParams();
}

ThunkParams* ThunkParams::createCopyParams()
{
	assert( 0 == 1);
}

ThunkParams* ThunkParams::createSTUB(void)
{
	assert (0 == 1);
}


const std::string ThunkParams::getFCallName(void) const
{
	if (no_params) 
		return THUNKPARAM_EMPTYNAME;

	assert (0 == 1);
}

