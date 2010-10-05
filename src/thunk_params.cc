#include "thunk_params.h"
#include "code_builder.h"
#include "runtime_interface.h"
#include "thunk_type.h"

extern CodeBuilder*	code_builder;
extern RTInterface	rt_glue;

using namespace std;

static bool has_gen_empty_proto = false;
static bool has_gen_empty_code = false; 

FCall* ThunkParams::copyFCall(void) const
{
	ExprList	*fc_exprs;
	const Type	*t;

	t = getOwner()->getType();

	fc_exprs = new ExprList(
		rt_glue.getThunkClosure(),
		new FCall(
			new Id("paramsAllocaByCount"), 
			new ExprList(
			new Number(
				(t != NULL) ? t->getNumArgs() : 0))));

	return new FCall(new Id(getFCallName()), fc_exprs);
}

ThunkParams* ThunkParams::copy(void) const
{
	ThunkParams	*ret;

	if (no_params) {
		ret = createNoParams();
	} else if (exprs != NULL) {
		ret = new ThunkParams(exprs->copy());
	} else {
		ret = NULL;
		assert (0 == 1 && "BAD THUNKPARAMS");
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

	/* diskoff,  parent's params, out params */
	args.push_back(code_builder->getClosureTyPtr());
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

	assert (exprs != NULL);
	return genCodeExprs();
}

bool ThunkParams::genCodeExprs(void) const 
{
	llvm::Function			*f;
	llvm::BasicBlock		*bb_entry;
	llvm::Module			*mod;
	llvm::IRBuilder<>		*builder;
	llvm::AllocaInst		*params_out_ptr;
	llvm::Function::arg_iterator	arg_it;
	ExprList::const_iterator	it;
	unsigned int			i;

	assert (exprs != NULL);

	cerr << "Starting genCodeExprs............" << endl;

	mod = code_builder->getModule();
	builder = code_builder->getBuilder();

	f = mod->getFunction(getFCallName());
	bb_entry = llvm::BasicBlock::Create(
		llvm::getGlobalContext(), "tp_entry", f);
	builder->SetInsertPoint(bb_entry);

	/* load n parameters */
	code_builder->genThunkHeaderArgs(f, getOwner()->getType());

	arg_it = f->arg_begin();	/* closure */
	arg_it++;			/* params (out) */

	params_out_ptr = builder->CreateAlloca(
		arg_it->getType(),
		0,
		"__thunkparams_out");
	builder->CreateStore(arg_it, params_out_ptr);
	

	/* assign all values in expr list to elements in 
	 * the passed pointer */
	for (it = exprs->begin(), i = 0; it != exprs->end(); it++, i++) {
		Expr		*cur_expr;
		llvm::Value	*expr_val;
		llvm::Value	*cur_param_ptr;
		llvm::Value	*idx_val;

		cur_expr = *it;
		expr_val = cur_expr->codeGen();
		if (expr_val == NULL) {
			cerr << "Oops. Could not generate ";
			cur_expr->print(cerr);
			cerr << " for thunkparams: (";
			exprs->print(cerr);
			cerr << ')' << endl;
			assert (0 == 1);
			return false;
		}
	
		idx_val = llvm::ConstantInt::get(
			llvm::getGlobalContext(),
			llvm::APInt(32, i));
		cur_param_ptr = builder->CreateGEP(
			builder->CreateLoad(params_out_ptr), idx_val);
		
		builder->CreateStore(expr_val, cur_param_ptr);
	}
		
	builder->CreateRetVoid();

	cerr << "GenCodeExprs DONE" << endl;

	return true;
}

ThunkParams* ThunkParams::createNoParams()
{
	/* used when there are no parameters assocaited with a field */
	return new ThunkParams();
}

ThunkParams* ThunkParams::createCopyParams()
{
	assert( 0 == 1);
	return NULL;
}

ThunkParams* ThunkParams::createSTUB(void)
{
	assert (0 == 1);
	return NULL;
}

const std::string ThunkParams::getFCallName(void) const
{
	if (no_params) 
		return THUNKPARAM_EMPTYNAME;

	return ThunkFieldFunc::getFCallName();
}

ThunkParams::~ThunkParams(void)
{
	if (exprs != NULL) delete exprs;
}
