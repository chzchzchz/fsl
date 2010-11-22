#include "thunk_params.h"
#include "code_builder.h"
#include "runtime_interface.h"
#include "thunk_type.h"
#include "typeclosure.h"
#include "eval.h"

extern type_map		types_map;
extern symtab_map       symtabs;
extern CodeBuilder*	code_builder;
extern RTInterface	rt_glue;

using namespace std;

static bool has_gen_empty_proto = false;
static bool has_gen_empty_code = false; 

FCall* ThunkParams::copyFCall(void) const
{
	ExprList	*fc_exprs;
	const Type	*t;

	t = getFieldType();

	fc_exprs = new ExprList();
	fc_exprs->add(rt_glue.getThunkClosure());	/* parent closure */
	fc_exprs->add(rt_glue.getThunkArgIdx());
	fc_exprs->add(new FCall(			/* param output buffer */
			new Id("paramsAllocaByCount"),
			new ExprList(
			new Number(
				(t != NULL) ? t->getParamBufEntryCount() : 0))));

	return new FCall(new Id(getFCallName()), fc_exprs);
}

FCall* ThunkParams::copyFCall(unsigned int idx) const
{
	ExprList	*fc_exprs;
	const Type	*t;

	t = getFieldType();

	fc_exprs = new ExprList();
	fc_exprs->add(rt_glue.getThunkClosure());	/* parent closure */
	fc_exprs->add(new Number(0));
	fc_exprs->add(new FCall(			/* param output buffer */
			new Id("paramsAllocaByCount"), 
			new ExprList(
			new Number(
				(t != NULL) ?
					t->getParamBufEntryCount() : 0))));

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
	ret->setFieldType(getFieldType());
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

	/* parent type instance, type's array idx, out params */
	args.push_back(code_builder->getClosureTyPtr());
	args.push_back(llvm::Type::getInt64Ty(llvm::getGlobalContext()));
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
	llvm::AllocaInst		*paramsf_idx;
	llvm::Function::arg_iterator	arg_it;

	mod = code_builder->getModule();
	builder = code_builder->getBuilder();

	f = mod->getFunction(getFCallName());
	bb_entry = llvm::BasicBlock::Create(
		llvm::getGlobalContext(), "tp_entry", f);
	builder->SetInsertPoint(bb_entry);

	/* load up __thunk_off_arg, __thunk_params, ... parameters */
	/* the things we expect while in type-scope */
	code_builder->genThunkHeaderArgs(f, getOwner()->getType());

	arg_it = f->arg_begin();	/* closure */

	/* map passed '@' to the Id '@' */
	arg_it++;			/* idx */
	assert (arg_it != f->arg_end());
	paramsf_idx = code_builder->createTmpI64(rt_glue.getThunkArgIdxName());
	assert (paramsf_idx != NULL);
	builder->CreateStore(arg_it, paramsf_idx);

	arg_it++;			/* params (out) */

	params_out_ptr = builder->CreateAlloca(
		arg_it->getType(),
		0,
		"__thunkparams_out");
	builder->CreateStore(arg_it, params_out_ptr);
	
	if (storeIntoParamBuf(params_out_ptr) == false)
		return false;

	builder->CreateRetVoid();

	return true;
}

bool ThunkParams::storeIntoParamBuf(llvm::AllocaInst *params_out_ptr) const
{
	const Type			*t;
	const ArgsList			*args;
	llvm::IRBuilder<>		*builder;
	EvalCtx				ectx(symtabs[getType()->getName()]);
	ExprList::const_iterator	it;
	unsigned int			pb_idx, arg_idx;

	assert (exprs != NULL);
	if ((t = getFieldType()) && t->getArgs() && exprs->size() == 0)
		assert (0 == 1 && "Type needs args, but none given");
	if (exprs->size() == 0) return true;

	builder = code_builder->getBuilder();

	assert (t != NULL && "Only types can take arguments");
	args = t->getArgs();
	assert (args != NULL && "Field has arguments, but type does not!");

	/* assign all values in expr list to elements in passed parambuf ptr */
	pb_idx = 0;
	for (	it = exprs->begin(), arg_idx = 0;
		it != exprs->end();
		it++, arg_idx++)
	{
		Expr		*cur_expr, *idxed_expr;
		llvm::Value	*expr_val;
		arg_elem	cur_arg;

		/* generate code for idx */
		cur_expr = *it;
		idxed_expr = Expr::rewriteReplace(
			eval(ectx, cur_expr),
			new Id("@"),
			rt_glue.getThunkArgIdx());
		expr_val = idxed_expr->codeGen();
		delete idxed_expr;

		if (expr_val == NULL) {
			cerr << "Oops. Could not generate ";
			cur_expr->print(cerr);
			cerr << " for thunkparams: (";
			exprs->print(cerr);
			cerr << ')' << endl;
			assert (0 == 1);
			return false;
		}

		cur_arg = args->get(arg_idx);
		if (types_map.count((cur_arg.first)->getName()) > 0) {
			const Type	*arg_type;
			arg_type = types_map[(cur_arg.first)->getName()];
			storeClosureIntoParamBuf(
				params_out_ptr, pb_idx,
				arg_type, expr_val);
			pb_idx += arg_type->getParamBufEntryCount() + 2;
		} else {
			builder->CreateStore(
				expr_val,
				code_builder->loadPtr(params_out_ptr, pb_idx));
			pb_idx++;
		}
	}

	return true;
}

/* load closure into parambuf */
void ThunkParams::storeClosureIntoParamBuf(
	llvm::AllocaInst* params_out_ptr, unsigned int pb_idx,
	const Type* t, llvm::Value* clo) const
{
	llvm::IRBuilder<>	*builder;
	llvm::Value		*cur_param_ptr;
	const ArgsList		*args;
	int			ent_c;
	TypeClosure		tc(clo);

	builder= code_builder->getBuilder();

	/* offset */
	cur_param_ptr = code_builder->loadPtr(params_out_ptr, pb_idx);
	builder->CreateStore(tc.getOffset(), cur_param_ptr);

	/* xlate */
	cur_param_ptr = code_builder->loadPtr(params_out_ptr, pb_idx+1);
	builder->CreateStore(
		builder->CreatePtrToInt(
			tc.getXlate(),
			builder->getInt64Ty(),
			"xlate_64cast"),
		cur_param_ptr);

	args = t->getArgs();
	if (args == NULL) return;

	ent_c = t->getParamBufEntryCount();
	assert (ent_c >= 0 && "Has args but <= 2 parambuf ents??");
	/* copy elements from clo's parambuf into param_out_ptr */
	code_builder->emitMemcpy64(
		code_builder->loadPtr(params_out_ptr, pb_idx+2),
		tc.getParamBuf(),
		ent_c);
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
