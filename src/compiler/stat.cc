#include "stat.h"

#include <assert.h>
#include "symtab.h"
#include "code_builder.h"
#include "struct_writer.h"
#include "evalctx.h"
#include "util.h"

using namespace std;

extern CodeBuilder		*code_builder;
extern symtab_map		symtabs;

#define STAT_NUM_ARGS		5

Stat::Stat(const Type* t) : Annotation(t, "stat") { loadByName(); }
Stat::~Stat(void) {}

void Stat::load(const Preamble* p)
{
	const preamble_args		*args;
	preamble_args::const_iterator	it;

	args = p->getArgsList();
	if (args == NULL || args->size() != STAT_NUM_ARGS) {
		cerr << "stat: expects five arguments" << endl;
		return;
	}

	for (it = args->begin(); it != args->end(); it++) {
		CondOrExpr	*coe = *it;
		if (coe->getExpr() == NULL) {
			cerr << "stat: only exprs" << endl;
			return;
		}
	}

	stat_elems.add(new StatEnt(this, p));
}

StatEnt::StatEnt(Stat* in_parent, const Preamble* in_pre)
: AnnotationEntry(in_parent, in_pre) { }

void StatEnt::genCode(void)
{
	llvm::Function::arg_iterator	arg_it;
	llvm::Function		*f;
	llvm::BasicBlock	*entry_bb;
	llvm::IRBuilder<>	*builder;
	llvm::AllocaInst	*pb_out;
	const ArgsList		*args_out;
	ExprList		*exprs;
	EvalCtx			ec(symtabs[parent->getType()->getName()]);

	f = code_builder->getModule()->getFunction(getFCallName());
	assert (f != NULL && "Missing stat prototype");

	entry_bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry", f);
	builder = code_builder->getBuilder();
	builder->SetInsertPoint(entry_bb);

	/* 1. pull in type scope */
	code_builder->genThunkHeaderArgs(f, parent->getType());

	/* 2. load up output parambuf */
	arg_it = f->arg_begin();
	arg_it++;
	pb_out = code_builder->createTmpI64Ptr();
	builder->CreateStore(arg_it, pb_out);

	/* 3. dump expressions into parambuf */
	args_out = new ArgsList(STAT_NUM_ARGS);
	exprs = pre->toExprList();
	code_builder->storeExprListIntoParamBuf(&ec, args_out, exprs, pb_out);
	delete args_out;
	delete exprs;

	/* 4. return */
	builder->CreateRetVoid();
}

void StatEnt::genProto(void)
{
	code_builder->genProto(
		getFCallName(),
		NULL,
		code_builder->getClosureTyPtr(),
		code_builder->getI64TyPtr());
}

void StatEnt::genInstance(TableGen* tg) const
{
	StructWriter	sw(tg->getOS());
	sw.write("st_statf", getFCallName());
	writeName(sw, "st_name");
}

void Stat::genExterns(TableGen* tg)
{
	const string args[] = {"const struct fsl_rt_closure*", "uint64_t*"};
	for (	statent_list::const_iterator it = stat_elems.begin();
		it != stat_elems.end();
		it++)
	{
		tg->printExternFunc(
			(*it)->getFCallName(),
			"void",
			vector<string>(args, args+2));
	}
}

void Stat::genTables(TableGen* tg)
{
	StructWriter	sw(
		tg->getOS(),
		"fsl_rtt_stat",
		"__rt_tab_stats_" + getType()->getName() + "[]",
		true);

	for (	statent_list::const_iterator it = stat_elems.begin();
		it != stat_elems.end();
		it++)
	{
		sw.beginWrite();
		(*it)->genInstance(tg);
	}
}
