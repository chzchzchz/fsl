#include <assert.h>
#include "wpkt_call.h"
#include "code_builder.h"
#include "evalctx.h"
#include "eval.h"
#include "util.h"
#include "struct_writer.h"

extern CodeBuilder	*code_builder;
extern writepkt_map	writepkts_map;

using namespace std;


void WritePktCall::printExterns(TableGen* tg) const
{
	const string args_w2w[] = {"const uint64_t*", "uint64_t*"};
	const string args_c[] = {"const uint64_t*"};

	tg->printExternFunc(
		getFuncName(), "void", vector<string>(args_w2w, args_w2w+2));
	tg->printExternFunc(
		getCondFuncName(), "bool", vector<string>(args_c, args_c+1));
}

/**
 * need to generate protos for
 *	1. wpkt2wpkt param conversion
 *	2. conditional (if none exists, ignore it)
 */
void WritePktCall::genProto(void) const
{
	/* two args: params in, params out */
	code_builder->genProto(
		getFuncName(),
		NULL,
		llvm::Type::getInt64PtrTy(llvm::getGlobalContext()),
		llvm::Type::getInt64PtrTy(llvm::getGlobalContext()));

	/* one arg: just the params */
	code_builder->genProto(
		getCondFuncName(),
		llvm::Type::getInt1Ty(llvm::getGlobalContext()),
		llvm::Type::getInt64PtrTy(llvm::getGlobalContext()));
}

std::string WritePktCall::getCondFuncName(void) const
{
	return getFuncName() + "_cond";
}


void WritePktCall::genTableInstance(class TableGen* tg) const
{
	StructWriter	sw(tg->getOS());
	sw.write("w2w_params_f", getFuncName());
	sw.write("w2w_cond_f", getCondFuncName());
	sw.write("w2w_wpkt",  string("&wpkt_")+name->getName()+int_to_string(0));
}

void WritePktCall::genCodeWpkt2Wpkt(void) const
{
	llvm::Function			*f;
	llvm::BasicBlock		*entry_bb;
	llvm::IRBuilder<>		*builder;
	llvm::Function::arg_iterator	arg_it;
	llvm::AllocaInst		*pb_out;
	const ArgsList			*args_out;
	WritePkt			*call_pkt;
	EvalCtx				ec(
		getParent()->getParent()->getVarScope());

	if (writepkts_map.count(name->getName()) == 0) {
		cerr << "NO WRITE PACKET BY NAME OF " <<
			name->getName() << endl;
		return;
	}
	call_pkt = writepkts_map[name->getName()];

	args_out = call_pkt->getArgs();
	f = code_builder->getModule()->getFunction(getFuncName());
	if (f == NULL) {
		cerr	<< "Could not find function prototype for "
			<< getFuncName() << endl;
		return;
	}

	entry_bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry", f);
	builder = code_builder->getBuilder();
	builder->SetInsertPoint(entry_bb);

	/* 1. extract and name arguments from argument array */
	getParent()->getParent()->genLoadArgs(f);

	/* 2. load up output parambuf */
	arg_it = f->arg_begin();
	arg_it++;
	pb_out = code_builder->createTmpI64Ptr();
	builder->CreateStore(arg_it, pb_out);

	/* 3. dump expressions into parambuf */
	code_builder->storeExprListIntoParamBuf(&ec, args_out, exprs, pb_out);

	builder->CreateRetVoid();
}

void WritePktCall::genCodeCond(void) const
{
	llvm::Function			*f;
	llvm::BasicBlock		*bb_then, *bb_else, *entry_bb;
	llvm::IRBuilder<>		*builder;

	builder = code_builder->getBuilder();
	f = code_builder->getModule()->getFunction(getCondFuncName());

	entry_bb = llvm::BasicBlock::Create(
		llvm::getGlobalContext(), "entry", f);

	bb_then = llvm::BasicBlock::Create(
		llvm::getGlobalContext(), "wpktc_then", f);
	builder->SetInsertPoint(bb_then);
	builder->CreateRet(
		llvm::ConstantInt::get(
			llvm::getGlobalContext(), llvm::APInt(1, 1)));

	bb_else = llvm::BasicBlock::Create(
		llvm::getGlobalContext(), "wpktc_else", f);
	builder->SetInsertPoint(bb_else);
	builder->CreateRet(
		llvm::ConstantInt::get(
			llvm::getGlobalContext(), llvm::APInt(1,0)));

	WritePktStmt::genCodeHeader(f, entry_bb, bb_then, bb_else);
}

void WritePktCall::genCode(void) const
{
	genCodeCond();
	genCodeWpkt2Wpkt();
}

string WritePktCall::getFuncName(void) const
{
	return  "__wpktcall_"+parent->getParent()->getName() +
		"_b" + int_to_string(parent->getBlkNum()) +
		"_s" + int_to_string(stmt_num);
}

std::ostream& WritePktCall::print(std::ostream& out) const
{
	out << "(writepkt-call ";
	name->print(out); out << ' '; exprs->print(out); out << ')';
	return out;
}

WritePktCall::WritePktCall(Id* in_name, ExprList* in_exprs)
: name(in_name), exprs(in_exprs)
{ }
