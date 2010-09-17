#include "code_builder.h"
#include "instance_iter.h"
#include "symtab.h"
#include "eval.h"

using namespace std;

extern CodeBuilder* 	code_builder;
extern symtab_map	symtabs;


InstanceIter::~InstanceIter(void)
{
	delete binding;
	delete min_expr;
	delete max_expr;
	delete lookup_expr;
}

InstanceIter::InstanceIter(
	const Type*	in_src_type,
	const Type*	in_dst_type,
	Id*		in_binding,
	Expr*		in_min_expr,
	Expr*		in_max_expr,
	Expr*		in_lookup_expr)
: src_type(in_src_type), dst_type(in_dst_type),
  binding(in_binding),
  min_expr(in_min_expr), max_expr(in_max_expr),
  lookup_expr(in_lookup_expr)
{
	assert (src_type != NULL);
	assert (dst_type != NULL);
	assert (binding != NULL);
	assert (min_expr != NULL);
	assert (max_expr != NULL);
	assert (lookup_expr != NULL);
}

string InstanceIter::getLookupFCallName(void) const
{
	return fc_name_prefix + "_II_lookup";
}

string InstanceIter::getMinFCallName(void) const
{
	return fc_name_prefix + "_II_min";
}

string InstanceIter::getMaxFCallName(void) const
{
	return fc_name_prefix + "_II_max";
}

void InstanceIter::genCode(void) const
{
	/* function(<thunk-args>, <binding-var>) */
	genCodeLookup();

	/* function(<thunk-args>) */
	code_builder->genCode(src_type, getMinFCallName(), min_expr);
	code_builder->genCode(src_type, getMaxFCallName(), max_expr);
}

void InstanceIter::genCodeLookup(void) const
{
	llvm::Function		*f;
	llvm::BasicBlock	*bb_bits;
	llvm::IRBuilder<>	*builder;
	llvm::Value		*ret_typepass, *ret_off, *ret_params;
	string			fcall_bits;
	Expr			*expr_eval_bits;
	SymbolTable		*local_syms;
	string			fname;
	Expr			*raw_expr = lookup_expr;
	llvm::Function::arg_iterator	ai;
	llvm::Module		*mod;


	mod = code_builder->getModule();
	fname = getLookupFCallName();
	builder = code_builder->getBuilder();

	assert (raw_expr != NULL);

	f = mod->getFunction(fname);
	local_syms = symtabs[src_type->getName()];
	assert (local_syms != NULL);

	expr_eval_bits = eval(EvalCtx(local_syms), raw_expr);
	assert (expr_eval_bits != NULL);

	bb_bits = llvm::BasicBlock::Create(
		llvm::getGlobalContext(), "entry", f);
	builder->SetInsertPoint(bb_bits);
	code_builder->genThunkHeaderArgs(f, src_type);

	ai = f->arg_begin();	/* closure */
	ai++;			/* idx */

	/* gen s for idx and output parambuf */
	builder->CreateStore(
		ai,
		code_builder->createTmpI64(binding->getName()));

	ret_typepass = expr_eval_bits->codeGen();
	assert (ret_typepass != NULL);

	assert (ret_typepass->getType() == code_builder->getClosureTyPtr());
	/* output value should be a typepass struct */

	ret_params = builder->CreateExtractValue(
		builder->CreateLoad(ret_typepass), 1, "params");
	ai++;	/* parambuf output */
	code_builder->emitMemcpy64(ai, ret_params, dst_type->getNumArgs());

	ret_off = builder->CreateExtractValue(
		builder->CreateLoad(ret_typepass), 0, "offset");
	builder->CreateRet(ret_off);
}

void InstanceIter::genProto(void) const
{
	const ThunkType	*tt;

	tt = symtabs[src_type->getName()]->getThunkType();
	{
	llvm::Function			*f;
	llvm::FunctionType		*ft;
	vector<const llvm::Type*>	args;

	/* closure */
	args.push_back(code_builder->getClosureTyPtr());

	/* binding sym */
	args.push_back(llvm::Type::getInt64Ty(llvm::getGlobalContext()));

	/* parambuf_t out */
	args.push_back(llvm::Type::getInt64PtrTy(llvm::getGlobalContext()));

	ft = llvm::FunctionType::get(
		llvm::Type::getInt64Ty(llvm::getGlobalContext()), args, false);
	f = llvm::Function::Create(
		ft,
		llvm::Function::ExternalLinkage,
		getLookupFCallName(),
		code_builder->getModule());

	/* should not be redefinitions.. */
	if (f->getName() != getLookupFCallName()) {
		cerr << "Expected name " << getLookupFCallName() <<" got " <<
		f->getNameStr() << endl;
	}

	assert (f->getName() == getLookupFCallName());
	assert (f->arg_size() == args.size());
	}

	code_builder->genThunkProto(getMinFCallName());
	code_builder->genThunkProto(getMaxFCallName());
}
