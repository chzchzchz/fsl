#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Intrinsics.h>
#include <fstream>

#include "eval.h"
#include "evalctx.h"
#include "symtab.h"
#include "expr.h"
#include "func.h"
#include "cond.h"
#include "runtime_interface.h"
#include "code_builder.h"
#include "varscope.h"
#include "util.h"

using namespace std;

extern symtab_map	symtabs;
extern RTInterface	rt_glue;

CodeBuilder::CodeBuilder(const char* mod_name) : tmp_c(0)
{
	builder = new llvm::IRBuilder<>(llvm::getGlobalContext());
	mod = new llvm::Module(mod_name, llvm::getGlobalContext());
	debug_output = false;
	vscope = new VarScope(builder);
	makeClosureTy();
}

CodeBuilder::~CodeBuilder(void)
{
	delete vscope;
	delete mod;
	delete builder;
}

unsigned int CodeBuilder::getTmpVarCount(void) const
{
	return vscope->size();
}

void CodeBuilder::createGlobal(const char* str, uint64_t v, bool is_const)

{
	llvm::GlobalVariable	*gv;

	gv = new llvm::GlobalVariable(
		*mod,
		llvm::Type::getInt64Ty(llvm::getGlobalContext()),
		is_const,
		llvm::GlobalVariable::ExternalLinkage,
		llvm::ConstantInt::get(
			llvm::getGlobalContext(), 
			llvm::APInt(64, v, false)),
		str);
}

llvm::GlobalVariable* CodeBuilder::getGlobalVar(const std::string& varname) const
{
	llvm::GlobalVariable	*gv;

	gv = mod->getGlobalVariable(varname);
	if (gv == NULL)
		return NULL;

	return gv;
}


void CodeBuilder::genProto(const string& name, uint64_t num_args)
{
	llvm::Function		*f;
	llvm::FunctionType	*ft;

	vector<const llvm::Type*>	args(
		num_args,
		llvm::Type::getInt64Ty(llvm::getGlobalContext()));

	ft = llvm::FunctionType::get(
		llvm::Type::getInt64Ty(llvm::getGlobalContext()),
		args,
		false);

	f = llvm::Function::Create(
		ft,
		llvm::Function::ExternalLinkage, 
		name,
		mod);

	/* should not be redefinitions.. */
	if (f->getName() != name) {
		cerr << "Expected name " << name <<" got " <<
		f->getNameStr() << endl;
	}
	assert (f->getName() == name);
	assert (f->arg_size() == args.size());
}


void CodeBuilder::genThunkProto(
	const std::string&	name,
	const llvm::Type	*ret_type)
{
	llvm::Function			*f;
	llvm::FunctionType		*ft;
	vector<const llvm::Type*>	args;

	args.push_back(getClosureTyPtr());

	ft = llvm::FunctionType::get(ret_type, args, false);
	f = llvm::Function::Create(
		ft,
		llvm::Function::ExternalLinkage, 
		name,
		mod);

	/* should not be redefinitions.. */
	if (f->getName() != name) {
		cerr << "Expected name " << name <<" got " <<
		f->getNameStr() << endl;
	}

	assert (f->getName() == name);
	assert (f->arg_size() == args.size());
}


/** generate the prototype for a thunkfunction 
 * (e.g. f(diskoff_t, parambuf_t) */
void CodeBuilder::genThunkProto(const std::string& name)
{
	genThunkProto(name, builder->getInt64Ty());
}

void CodeBuilder::genCode(
	const Type* type,
	const string& fname,
	const Expr* raw_expr,
	const ArgsList* extra_args)
{
	llvm::Function		*f_bits;
	llvm::BasicBlock	*bb_bits;
	string			fcall_bits;
	Expr			*expr_eval_bits;
	SymbolTable		*local_syms;

	assert (raw_expr != NULL);

	f_bits = mod->getFunction(fname);
	local_syms = symtabs[type->getName()];
	assert (local_syms != NULL);

	expr_eval_bits = eval(EvalCtx(local_syms), raw_expr);

	bb_bits = llvm::BasicBlock::Create(
		llvm::getGlobalContext(), "entry", f_bits);
	builder->SetInsertPoint(bb_bits);
	genThunkHeaderArgs(f_bits, type, extra_args);
	builder->CreateRet(expr_eval_bits->codeGen());

	delete expr_eval_bits;
}


/* create empty function that returns void */
void CodeBuilder::genCodeEmpty(const std::string& name)
{
	llvm::Function		*f;
	llvm::BasicBlock	*bb;

	f = mod->getFunction(name);
	assert (f != NULL);

	bb = llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry", f);
	assert (bb != NULL);

	builder->SetInsertPoint(bb);
	builder->CreateRetVoid();
}

void CodeBuilder::genThunkHeaderArgs(
	llvm::Function* f, 
	const Type* t,
	const ArgsList* extra_args)
{
	llvm::Function::arg_iterator	ai;
	llvm::IRBuilder<>		tmpB(
		&f->getEntryBlock(), f->getEntryBlock().begin());

	assert (t != NULL);

	vscope->clear();

	/* closure arg */
	ai = f->arg_begin();
	vscope->loadClosureIntoThunkVars(ai, tmpB);
	ai++;
	/* now past the closure pointer */

	/* create the rest of the arguments */
	vscope->genTypeArgs(t, &tmpB);
	if (extra_args != NULL)
		vscope->genArgs(ai, &tmpB, extra_args);
}


void CodeBuilder::copyClosure(
	const Type* copy_type, llvm::Value *src_ptr, llvm::Value *dst_ptr)
{
	llvm::Value	*dst_params_ptr;
	llvm::Value	*src_params_ptr;
	llvm::Value*	src_loaded;
	llvm::Value	*copied_offset, *copied_xlate;

	src_loaded = builder->CreateLoad(src_ptr);

	/* copy contents of src into dst */
	copied_offset = builder->CreateInsertValue(
		builder->CreateLoad(dst_ptr),
		builder->CreateExtractValue(
			src_loaded,
			RT_CLO_IDX_OFFSET,
			"cb_copy_offset"),
		RT_CLO_IDX_OFFSET);

	copied_xlate = builder->CreateInsertValue(
		copied_offset,
		builder->CreateExtractValue(
			src_loaded,
			RT_CLO_IDX_XLATE,
			"cb_copy_xlate"),
		RT_CLO_IDX_XLATE);

	builder->CreateStore(copied_xlate, dst_ptr);

	src_params_ptr = builder->CreateExtractValue(
		src_loaded, RT_CLO_IDX_PARAMS, "src_params");
	dst_params_ptr = builder->CreateExtractValue(
		builder->CreateLoad(dst_ptr), RT_CLO_IDX_PARAMS, "dst_params");

	emitMemcpy64(dst_params_ptr, src_params_ptr, copy_type->getNumArgs());
}

void CodeBuilder::write(ostream& os)
{
	llvm::raw_os_ostream	llvm_out(os);
	mod->print(llvm_out, NULL);
}

void CodeBuilder::genCodeCond(
	const Type* t,
	const std::string& func_name,
	const CondExpr*	cond_expr,
	const Expr*	true_expr,
	const Expr*	false_expr)
{
	llvm::Function		*f;
	llvm::BasicBlock	*bb_then, *bb_else, *bb_entry;
	llvm::Value		*cond_v, *false_v, *true_v;
	llvm::LLVMContext	&gctx(llvm::getGlobalContext());
	EvalCtx			ectx(symtabs[t->getName()]);

	assert (true_expr != NULL);
	assert (false_expr != NULL);
	assert (cond_expr != NULL);

	f = mod->getFunction(func_name);

	bb_entry = llvm::BasicBlock::Create(gctx, "entry", f);
	builder->SetInsertPoint(bb_entry);
	genThunkHeaderArgs(f, t);

	cond_v = cond_codeGen(&ectx, cond_expr);
	if (cond_v == NULL) {
		cerr << func_name << ": could not gen condition" << endl;
		return;
	}

	bb_then = llvm::BasicBlock::Create(gctx, "codecond_then", f);
	bb_else = llvm::BasicBlock::Create(gctx, "codecond_else", f);

	/*
	 * if (cond_v) {
	 * 	(bb_then)
	 * 	jmp merge
	 * } else {
	 * 	(bb_else)
	 * 	jmp merge
	 * }
	 * merge:
	 */
	/* then.. */
	builder->CreateCondBr(cond_v, bb_then, bb_else);
	builder->SetInsertPoint(bb_then);
	true_v = evalAndGen(ectx, true_expr);
	builder->CreateRet(true_v);

	/* else */
	builder->SetInsertPoint(bb_else);
	false_v = evalAndGen(ectx, false_expr);
	builder->CreateRet(false_v);
}

void CodeBuilder::write(std::string& os)
{
	ofstream	ofs(os.c_str());
	write(ofs);
}

llvm::Type* CodeBuilder::getClosureTyPtr(void)
{
	return llvm::PointerType::get(closure_struct, 0);
}

void CodeBuilder::makeClosureTy(void)
{
	vector<const llvm::Type*>	types;
	llvm::LLVMContext		&gctx(llvm::getGlobalContext());

	/* diskoffset */
	types.push_back(llvm::Type::getInt64Ty(gctx));
	/* param array */
	types.push_back(llvm::Type::getInt64PtrTy(gctx));
	/* clo_xlate ptr-- voidptr; opaque to llvm */
	types.push_back(llvm::Type::getInt8PtrTy(gctx));

	closure_struct = llvm::StructType::get(gctx, types, "closure");
}

llvm::AllocaInst* CodeBuilder::createTmpI64Ptr(void)
{
	return vscope->createTmpI64Ptr();
}

llvm::AllocaInst* CodeBuilder::getTmpAllocaInst(const std::string& s) const
{
	return vscope->getTmpAllocaInst(s);
}

void CodeBuilder::printTmpVars(void) const
{
	vscope->print();
}

llvm::AllocaInst* CodeBuilder::createTmpI64(const std::string& name)
{
	return vscope->createTmpI64(name);
}

void CodeBuilder::emitMemcpy64(
	llvm::Value* dst, llvm::Value* src, unsigned int elem_c)
{
	llvm::Function	*memcpy_f;
	const llvm::Type *Tys[] = {
		builder->getInt8PtrTy(),
		builder->getInt8PtrTy(),
		builder->getInt32Ty(),
		builder->getInt32Ty()};
	llvm::Value* args[] = {
		/* dst */
	  	builder->CreateBitCast(
			dst, builder->getInt8PtrTy(), "dst_ptr"),

		/* src */
	  	builder->CreateBitCast(
			src, builder->getInt8PtrTy(), "src_ptr"),

		/* element bytes (8 per ...) */
		llvm::ConstantInt::get(
			llvm::getGlobalContext(), llvm::APInt(32, elem_c*8)),

		/* ailgnment */
		llvm::ConstantInt::get(builder->getInt32Ty(), 4),

		/* is volatile (no) */
		llvm::ConstantInt::get(builder->getInt1Ty(), 0),
	};

	memcpy_f =  llvm::Intrinsic::getDeclaration(
		mod, llvm::Intrinsic::memcpy, Tys, 3);
	builder->CreateCall(memcpy_f, args, args+5);
}


llvm::AllocaInst* CodeBuilder::createTmpI64(void)
{
	return createTmpI64("__BBB" + int_to_string(tmp_c++));
}

llvm::Value* CodeBuilder::getNullPtrI64(void)
{
	llvm::LLVMContext	&gctx(llvm::getGlobalContext());
	llvm::Value		*ret;
	ret = llvm::ConstantExpr::getIntToPtr(
		llvm::ConstantInt::get(llvm::Type::getInt64Ty(gctx), 0),
		llvm::PointerType::getUnqual(
			llvm::Type::getInt64Ty(gctx)));
	return ret;
}

llvm::Value* CodeBuilder::getNullPtrI8(void)
{
	llvm::LLVMContext	&gctx(llvm::getGlobalContext());
	llvm::Value		*ret;
	ret = llvm::ConstantExpr::getIntToPtr(
		llvm::ConstantInt::get(llvm::Type::getInt64Ty(gctx), 0),
		llvm::PointerType::getUnqual(
			llvm::Type::getInt8Ty(gctx)));
	return ret;
}

llvm::AllocaInst* CodeBuilder::createTmpClosure(
	const Type* t, const std::string& name)
{
	return vscope->createTmpClosure(t, name);
}

llvm::AllocaInst* CodeBuilder::createPrivateTmpI64Array(
	unsigned int num_elems,
	const std::string& name)
{
	llvm::Value	*elem_count;

	elem_count = llvm::ConstantInt::get(
		llvm::getGlobalContext(),
		llvm::APInt(32, num_elems));

	 return builder->CreateAlloca(
		llvm::Type::getInt64Ty(llvm::getGlobalContext()),
		elem_count,
		name);
}
