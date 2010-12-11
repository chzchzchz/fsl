#include "code_builder.h"
#include "instance_iter.h"
#include "struct_writer.h"
#include "symtab.h"
#include "eval.h"

using namespace std;

extern CodeBuilder* 	code_builder;
extern symtab_map	symtabs;
extern type_map		types_map;


InstanceIter::~InstanceIter(void)
{
	delete binding;
	delete min_expr;
	delete max_expr;
	delete lookup_expr;
}

InstanceIter::InstanceIter(void)
 : src_type(NULL), dst_type(NULL) {}

bool InstanceIter::load(
	const Type	*in_src_type,
	preamble_args::const_iterator& arg_it)
{

	EvalCtx		ectx(symtabs[in_src_type->getName()]);
	Expr		*_binding;

	assert (src_type == NULL && dst_type == NULL && "Already initailized");

	src_type = in_src_type;

	_binding = (*arg_it)->getExpr()->copy(); arg_it++;;
	min_expr =(*arg_it)->getExpr()->copy(); arg_it++;
	max_expr = (*arg_it)->getExpr()->copy(); arg_it++;
	lookup_expr = (*arg_it)->getExpr()->copy(); arg_it++;

	binding = dynamic_cast<Id*>(_binding);
	if (binding == NULL) {
		cerr << "Expected id for binding but got:";
		_binding->print(cerr);
		cerr << endl;
		return false;
	}

	dst_type = ectx.getType(lookup_expr);
	if (dst_type == NULL) {
		cerr << "Bad dest type in iterator: ";
		lookup_expr->print(cerr);
		cerr << "' in type " << src_type->getName() << endl;
		return false;
	}

	return true;
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
	assert (f != NULL);

	local_syms = symtabs[src_type->getName()];
	assert (local_syms != NULL);

	bb_bits = llvm::BasicBlock::Create(
		llvm::getGlobalContext(), "entry", f);
	builder->SetInsertPoint(bb_bits);
	code_builder->genThunkHeaderArgs(f, src_type);

	expr_eval_bits = eval(EvalCtx(local_syms), raw_expr);
	assert (expr_eval_bits != NULL);

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
	code_builder->emitMemcpy64(ai, ret_params, dst_type->getParamBufEntryCount());

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

void InstanceIter::setMinExpr(const Expr* e)
{
	assert (e != NULL);
	assert (e != min_expr);
	delete min_expr;
	min_expr = e->copy();
}

void InstanceIter::genTableInstance(TableGen* tg) const
{
	StructWriter		sw(tg->getOS());

	sw.write("it_type_dst", getDstType()->getTypeNum());
	sw.write("it_range", getLookupFCallName());
	sw.write("it_min", getMinFCallName());
	sw.write("it_max", getMaxFCallName());
}

void InstanceIter::printExterns(TableGen* tg) const
{
	string	args_pr[] = {
		"const struct fsl_rt_closure*", "uint64_t", "uint64_t*"};
	string	args_bound[] = {"const struct fsl_rt_closure*"};

	tg->printExternFunc(
		getLookupFCallName(),
		vector<string>(args_pr,args_pr+3),
		"uint64_t");

	tg->printExternFunc(
		getMinFCallName(),
		vector<string>(args_bound,args_bound+1),
		"uint64_t");

	tg->printExternFunc(
		getMaxFCallName(),
		vector<string>(args_bound,args_bound+1),
		"uint64_t");
}
