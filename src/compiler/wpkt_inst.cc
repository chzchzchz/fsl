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
: parent(in_parent), t(in_t), funcname(in_fname), exprs(in_exprs->copy()) {}

void WritePktInstance::genProto(void) const
{
	/* writepkt inst function calls take three arguments:
	 * 	1. source type closure
	 * 	2. parambuf for input
	 * 	3. parambuf for output
	 */
	code_builder->genProto(
		funcname,
		NULL,
		code_builder->getClosureTyPtr(),
		llvm::Type::getInt64PtrTy(llvm::getGlobalContext()),
		llvm::Type::getInt64PtrTy(llvm::getGlobalContext()));
}

void WritePktInstance::genExterns(TableGen* tg) const
{
	string	args_pr[] = {
		"const struct fsl_rt_closure*", "uint64_t*", "uint64_t*"};
	tg->printExternFunc(
		funcname,
		"void",
		vector<string>(args_pr,args_pr+3));
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
	llvm::AllocaInst		*pb_out;
	const ArgsList			*args_out;
	EvalCtx				ec(symtabs[t->getName()]);

	args_out = getParent()->getArgs();
	assert (args_out != NULL);
	assert (exprs->size() == args_out->size());

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
	if (args_in != NULL)
		code_builder->loadArgsFromParamBuf(arg_it, args_in);

	/* 2. load up output parambuf */
	arg_it++;
	pb_out = code_builder->createTmpI64Ptr();
	builder->CreateStore(arg_it, pb_out);

	/* 3. dump expressions into parambuf */
	code_builder->storeExprListIntoParamBuf(&ec, args_out, exprs, pb_out);

	/* 4. return */
	builder->CreateRetVoid();
}
