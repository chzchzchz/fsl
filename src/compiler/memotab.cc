#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/IRBuilder.h>
#include <iostream>
#include <assert.h>

#include "func.h"
#include "memotab.h"
#include "type.h"
#include "runtime_interface.h"
#include "table_gen.h"
#include "typeclosure.h"
#include "struct_writer.h"

extern type_map		types_map;
extern CodeBuilder	*code_builder;
extern RTInterface	rt_glue;

using namespace std;

void MemoTab::registerFunction(const Func* f)
{
	const std::string&	fname(f->getName());

	assert (f->getNumArgs() == 0);
	assert (datatab_idxs.count(fname) == 0 && "Double registered function");

	f_list.push_back(f);
	datatab_idxs[fname] = last_datatab_idx;
	last_datatab_idx += getNumTabEntries(f);
}

unsigned int MemoTab::getNumTabEntries(const class Func* f) const
{
	const Type		*t = f->getRetType();
	if (t == NULL) return 1;
	return RT_CLO_ELEMS + t->getParamBufEntryCount();
}

llvm::Value* MemoTab::memoFuncCall(const Func* f) const
{
	if (f->getRetType() == NULL) return memoCallPrim(f);
	return memoCallType(f);
}

class llvm::Value* MemoTab::memoCallPrim(const class Func* f) const
{
	llvm::IRBuilder<>	*builder = code_builder->getBuilder();
	int			idx = getFuncIdx(f);
	llvm::Value		*ptr, *ret;

	ptr = builder->CreateStructGEP(nullptr, memo_datatab, idx);
	ret = builder->CreateLoad(ptr);

	/* special case-- boolean is a single bit */
	if (f->getRet() == "bool") {
		ret = builder->CreateTrunc(
			ret, llvm::Type::getInt1Ty(llvm::getGlobalContext()));
	}

	return ret;
}

class llvm::Value* MemoTab::memoCallType(const class Func* f) const
{
	llvm::IRBuilder<>	*builder = code_builder->getBuilder();
	int			idx = getFuncIdx(f);
	llvm::Value		*tab_ptr;
	llvm::AllocaInst	*tmp_tp, *tmp_params;
	unsigned int		param_c;

	tab_ptr = builder->CreateStructGEP(nullptr, memo_datatab, idx);
	tab_ptr = builder->CreateBitCast(
		tab_ptr, code_builder->getClosureTyPtr());

	/* create temporary storage for closure */
	param_c = f->getRetType()->getParamBufEntryCount();
	tmp_tp = builder->CreateAlloca(code_builder->getClosureTy());
	tmp_params = code_builder->createPrivateTmpI64Array(param_c, "RETCLO");
	/* set parambuf to private array */
	TypeClosure	tc(tmp_tp);
	builder->CreateStore(tc.setParamBuf(tmp_params), tmp_tp);

	/* copy over and return temp closure */
	code_builder->copyClosure(f->getRetType(), tab_ptr, tmp_tp);
	return tmp_tp;
}

void MemoTab::allocTable(void)
{
	assert (memo_datatab == NULL);

	memo_datatab = new llvm::GlobalVariable(
		*code_builder->getModule(),
		llvm::ArrayType::get(
			llvm::Type::getInt64Ty(llvm::getGlobalContext()),
			last_datatab_idx),
		false,
		llvm::GlobalVariable::ExternalLinkage,
		NULL,
		rt_glue.getMemoTabName());
}

int MemoTab::getFuncIdx(const Func* f) const
{
	assert (datatab_idxs.count(f->getName()) != 0);
	return (*(datatab_idxs.find(f->getName()))).second;
}

bool MemoTab::canMemoize(const Func* f) const
{
	if (f == NULL) return false;
	if (f->getNumArgs() != 0) return false;
	return true;
}

void MemoTab::genTables(TableGen* tg) const
{
	ostream&			os(tg->getOS());
	funcidx_list::const_iterator	it;

	/* memoized data storage table */
	os	<< "uint64_t " << rt_glue.getMemoTabName()
		<< "[" << last_datatab_idx << "];" << endl;
	os	<< "unsigned int __fsl_memotab_sz = "
		<< last_datatab_idx << ";" << endl;

	/* memoized function table */
	os << "unsigned int __fsl_memof_c = " << f_list.size() << ";\n";

	StructWriter	sw_tab(os, "fsl_memo_t", "__fsl_memotab_funcs[]");
	for (	funcidx_list::const_iterator it = f_list.begin();
		it != f_list.end();
		it++)
	{
		const Func	*f = (*it);
		const Type	*t = f->getRetType();

		sw_tab.beginWrite();

		StructWriter	sw_ent(os);
		sw_ent.write(
			(t==NULL) ? "m_f.f_prim" : "m_f.f_type", f->getName());
		sw_ent.write32("m_typenum", (t == NULL) ? ~0 : t->getTypeNum());
		sw_ent.write32("m_tabidx", getFuncIdx(f));
	}
}
