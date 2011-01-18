#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/IRBuilder.h>
#include <iostream>
#include <assert.h>

#include "func.h"
#include "memotab.h"
#include "type.h"
#include "runtime_interface.h"
#include "table_gen.h"

extern type_map		types_map;
extern CodeBuilder	*code_builder;
extern RTInterface	rt_glue;

using namespace std;

void MemoTab::registerFunction(const Func* f)
{
	const std::string&	fname(f->getName());
	int			total_idx;
	assert (f->getNumArgs() == 0);
	assert (fidxs.count(fname) == 0 && "Double registered function");
	total_idx = fidxs.size();
	fidxs[fname] = total_idx;
	f_list.push_back(fname);
}

llvm::Value* MemoTab::memoFuncCall(const Func* f) const
{
	llvm::IRBuilder<>	*builder;
	llvm::Value		*ptr, *ret;
	int			idx;

	builder = code_builder->getBuilder();
	idx = getFuncIdx(f);
	ptr = builder->CreateStructGEP(memo_table, idx);

	/* XXX: make this not suck */
	ret = builder->CreateLoad(ptr);

	cerr << f->getName() << endl;
	if (f->getRet() == "bool") {
		ret = builder->CreateTrunc(
			ret,
			llvm::Type::getInt1Ty(llvm::getGlobalContext()));
	}

	return ret;
}

void MemoTab::allocTable(void)
{
	assert (memo_table == NULL);

	memo_table = new llvm::GlobalVariable(
		*code_builder->getModule(),
		(llvm::Type*)llvm::ArrayType::get(
			llvm::Type::getInt64Ty(llvm::getGlobalContext()),
			fidxs.size()),
		false,
		llvm::GlobalVariable::ExternalLinkage,
		NULL,
		rt_glue.getMemoTabName());
}

int MemoTab::getFuncIdx(const Func* f) const
{
	assert (fidxs.count(f->getName()) != 0);
	return (*(fidxs.find(f->getName()))).second;
}

bool MemoTab::canMemoize(const Func* f) const
{
	if (f == NULL) return false;
	if (f->getNumArgs() != 0) return false;
	if (types_map.count(f->getRet()) > 0) return false;
	return true;
}


void MemoTab::genTables(TableGen* tg) const
{
	ostream&	os(tg->getOS());
	int		total_funcs;
	funcidx_list::const_iterator	it;

	total_funcs = fidxs.size();
	os	<< "uint64_t " << rt_glue.getMemoTabName()
		<< "[" << total_funcs << "];" << endl;
	os	<< "int __fsl_memotab_sz = " << total_funcs << ";" << endl;

	os	<< "memof_t __fsl_memotab_funcs[" << total_funcs << "] = {";

	it = f_list.begin();
	for (int i = 0; i < total_funcs-1; i++) {
		os << (*it) << ", " << endl;
		it++;
	}
	os << (*it) << "};" << endl;
}
