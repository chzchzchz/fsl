#include "type.h"
#include "writepkt.h"
#include "wpkt_inst.h"
#include "struct_writer.h"
#include "util.h"
#include "evalctx.h"
#include "eval.h"

using namespace std;

extern CodeBuilder	*code_builder;
extern symtab_map	symtabs;

WritePktInstance::WritePktInstance(
	const WritePkt* in_parent,
	const class Type* in_t,
	const std::string& in_fname,
	const ExprList*	in_exprs)
: parent(in_parent), t(in_t), funcname(in_fname)
{
	exprs = in_exprs->copy();
}

void WritePktInstance::genProto(void) const
{
	llvm::Function			*f;
	llvm::FunctionType		*ft;
	vector<const llvm::Type*>	args;


	args.push_back(code_builder->getClosureTyPtr());
	args.push_back(llvm::Type::getInt64PtrTy(llvm::getGlobalContext()));
	args.push_back(llvm::Type::getInt64PtrTy(llvm::getGlobalContext()));

	/* writepkt inst function calls take three arguments:
	 * 	1. source type closure
	 * 	2. parambuf for input
	 * 	3. parambuf for output
	 */
	ft = llvm::FunctionType::get(
		llvm::Type::getVoidTy(llvm::getGlobalContext()),
		args,
		false);

	f = llvm::Function::Create(
		ft,
		llvm::Function::ExternalLinkage,
		funcname,
		code_builder->getModule());

	/* should not be redefinitions.. */
	if (f->getName() != funcname) {
		cerr << "Expected name " << funcname <<" got " <<
		f->getNameStr() << endl;
	}

	assert (f->getName() == funcname);
	assert (f->arg_size() == 3);
}

void WritePktInstance::genExterns(TableGen* tg) const
{
	string	args_pr[] = {
		"const struct fsl_rt_closure*", "uint64_t*", "uint64_t*"};
	tg->printExternFunc(
		funcname,
		vector<string>(args_pr,args_pr+3),
		"void");
}

void WritePktInstance::genTableInstance(TableGen* tg) const
{
	StructWriter	sw(tg->getOS());

	sw.write("wpi_params", funcname);
	sw.write(	"wpi_wpkt",
			string("&wpkt_")+parent->getName()+int_to_string(0));
}

unsigned int WritePktInstance::getParamBufEntries(void) const
{
	return getParent()->getArgs()->getNumParamBufEntries();
}

void WritePktInstance::genCode(const ArgsList* args_in /* bindings */) const
{
	/* 0. create function / first bb */
	llvm::Function			*f;
	llvm::BasicBlock		*entry_bb;
	llvm::IRBuilder<>		*builder;
	llvm::Function::arg_iterator	arg_it;
	llvm::AllocaInst		*pb_out_ptr;
	const ArgsList			*args_out;
	unsigned int			pb_idx, arg_idx;
	EvalCtx				ectx(symtabs[t->getName()]);

	args_out = getParent()->getArgs();
	assert (args_out != NULL);
	assert (exprs->size() == args_out->size());

	cerr << "Generating code for wpktinstance " << funcname << endl;

	f = code_builder->getModule()->getFunction(funcname);
	assert (f->arg_size() == 3);
	assert (f != NULL && "Missing wpktinst prototype");

	entry_bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry", f);
	builder = code_builder->getBuilder();
	builder->SetInsertPoint(entry_bb);

	/* 1. pull in type scope */
	code_builder->genThunkHeaderArgs(f, t);

	/* 1.5. pull in input parambuf */
	arg_it = f->arg_begin();
	arg_it++;
	code_builder->loadArgsFromParamBuf(arg_it, args_in);

	/* 2. load up output parambuf */
	arg_it++;
	pb_out_ptr = code_builder->createTmpI64Ptr();
	builder->CreateStore(arg_it, pb_out_ptr);

	/* 3. dump expressions into parambuf */
	pb_idx = 0;
	arg_idx = 0;
	for (	ExprList::const_iterator it = exprs->begin();
		it != exprs->end();
		it++, arg_idx++)
	{
		const Expr	*cur_expr = *it;
		Expr		*evaled_cexpr;
		int		elems_stored;

		evaled_cexpr = eval(ectx, cur_expr);
		elems_stored = code_builder->storeExprIntoParamBuf(
			args_out->get(arg_idx), evaled_cexpr,
			pb_out_ptr, pb_idx);
		delete evaled_cexpr;
		if (elems_stored <= 0) {
			assert (0 == 1 && "FAILED TO STORE");
			return;
		}
		pb_idx += elems_stored;
	}

	/* 4. return */
	builder->CreateRetVoid();
}
