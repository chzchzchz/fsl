#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Support/raw_os_ostream.h>
#include <fstream>

#include "eval.h"
#include "evalctx.h"
#include "symtab.h"
#include "expr.h"
#include "func.h"
#include "cond.h"
#include "runtime_interface.h"
#include "code_builder.h"

using namespace std;

extern llvm_var_map	thunk_var_map;
extern symtab_map	symtabs;
extern const_map	constants;
extern RTInterface	rt_glue;

CodeBuilder::CodeBuilder(const char* mod_name)
{
	builder = new llvm::IRBuilder<>(llvm::getGlobalContext());
	mod = new llvm::Module(mod_name, llvm::getGlobalContext());
	debug_output = false;
}

CodeBuilder::~CodeBuilder(void)
{
	delete mod;
	delete builder;
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
	assert (f->getName() == name);
	assert (f->arg_size() == args.size());
}

void CodeBuilder::genCode(
	const Type* type,
	const string& fname,
	const Expr* raw_expr,
	const FuncArgs* extra_args)
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

	expr_eval_bits = eval(
		EvalCtx(local_syms, symtabs, constants),
		raw_expr);
	if (debug_output) {
		cerr << "DEBUG: " << fname << endl;
		expr_eval_bits->print(cerr);
		cerr << endl;
	}

	bb_bits = llvm::BasicBlock::Create(
		llvm::getGlobalContext(), "entry", f_bits);
	builder->SetInsertPoint(bb_bits);
	genHeaderArgs(f_bits, type, extra_args);
	builder->CreateRet(expr_eval_bits->codeGen());

	delete expr_eval_bits;
}

void CodeBuilder::genHeaderArgs(
	llvm::Function* f, 
	const Type* t,
	const FuncArgs* extra_args)
{
	const llvm::Type		*l_t;
	llvm::AllocaInst		*allocai;
	llvm::Function::arg_iterator	ai;
	llvm::IRBuilder<>		tmpB(
		&f->getEntryBlock(), f->getEntryBlock().begin());

	assert (t != NULL);

	l_t = llvm::Type::getInt64Ty(llvm::getGlobalContext());
	ai = f->arg_begin();

	thunk_var_map.clear();

	/* create the hidden argument __thunk_off_arg */
	allocai = tmpB.CreateAlloca(l_t, 0, rt_glue.getThunkArgName());
	thunk_var_map[rt_glue.getThunkArgName()] = allocai;
	thunk_var_map[t->getName()] = allocai;	/* alias for typename */
	builder->CreateStore(ai, allocai);
	
	/* skip over the first function argument (already used) */
	ai++;

	/* create the rest of the arguments */
	genArgs(ai, &tmpB, t->getArgs());
	if (extra_args != NULL) {
		genArgs(ai, &tmpB, extra_args->getArgsList());
	}
}

void CodeBuilder::genArgs(
	llvm::Function::arg_iterator	&ai,
	llvm::IRBuilder<>		*tmpB,
	const ArgsList			*args)
{
	unsigned int			arg_c;
	const llvm::Type		*l_t;

	if (args == NULL)
		return;

	l_t = llvm::Type::getInt64Ty(llvm::getGlobalContext());
	arg_c = args->size();

	for (unsigned int i = 0; i < arg_c; i++, ai++) {
		string				arg_name;
		llvm::AllocaInst		*allocai;

		arg_name = (args->get(i).second)->getName();
		allocai = tmpB->CreateAlloca(l_t, 0, arg_name);
		builder->CreateStore(ai, allocai);
		thunk_var_map[arg_name] = allocai;
	}
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
	llvm::BasicBlock	*bb_then, *bb_else, *bb_merge, *bb_entry;
	llvm::Value		*cond_v, *false_v, *true_v;
	llvm::LLVMContext	&gctx(llvm::getGlobalContext());
	EvalCtx			ectx(symtabs[t->getName()], symtabs, constants);

	assert (true_expr != NULL);
	assert (false_expr != NULL);
	assert (cond_expr != NULL);

	f = mod->getFunction(func_name);

	bb_entry = llvm::BasicBlock::Create(gctx, "entry", f);
	builder->SetInsertPoint(bb_entry);
	genHeaderArgs(f, t);

	cond_v = cond_codeGen(&ectx, cond_expr);
	if (cond_v == NULL) {
		cerr << func_name << ": could not gen condition" << endl;
		return;
	}

	bb_then = llvm::BasicBlock::Create(gctx, "then");
	bb_else = llvm::BasicBlock::Create(gctx, "else");

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

	f->getBasicBlockList().push_back(bb_then);
	f->getBasicBlockList().push_back(bb_else);
}


void CodeBuilder::write(std::string& os)
{
	ofstream	ofs(os.c_str());
	write(ofs);
}

