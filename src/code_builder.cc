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
#include "util.h"

using namespace std;

extern symtab_map	symtabs;
extern RTInterface	rt_glue;

CodeBuilder::CodeBuilder(const char* mod_name) : tmp_c(0)
{
	builder = new llvm::IRBuilder<>(llvm::getGlobalContext());
	mod = new llvm::Module(mod_name, llvm::getGlobalContext());
	debug_output = false;
	makeTypePassStruct();
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

	/* diskoff_t */
	args.push_back(llvm::Type::getInt64Ty(llvm::getGlobalContext()));

	/* parambuf_t */
	args.push_back(llvm::Type::getInt64PtrTy(llvm::getGlobalContext()));

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

	expr_eval_bits = eval(EvalCtx(local_syms), raw_expr);
	if (debug_output) {
		cerr << "DEBUG: " << fname << endl;
		expr_eval_bits->print(cerr);
		cerr << endl;
	}

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
	const FuncArgs* extra_args)
{
	const llvm::Type		*l_t;
	llvm::AllocaInst		*allocai;
	llvm::Function::arg_iterator	ai;
	llvm::IRBuilder<>		tmpB(
		&f->getEntryBlock(), f->getEntryBlock().begin());

	assert (t != NULL);

	tmp_var_map.clear();

	ai = f->arg_begin();

	/* create the hidden argument __thunk_off_arg */
	l_t = llvm::Type::getInt64Ty(llvm::getGlobalContext());
	allocai = tmpB.CreateAlloca(l_t, 0, rt_glue.getThunkArgOffsetName());
	tmp_var_map[rt_glue.getThunkArgOffsetName()] = allocai;
	tmp_var_map[t->getName()] = allocai;	/* alias for typename */
	builder->CreateStore(ai, allocai);

	/* skip over __thunk_off_arg */
	ai++;

	l_t = llvm::Type::getInt64PtrTy(llvm::getGlobalContext());
	allocai = tmpB.CreateAlloca(l_t, 0, rt_glue.getThunkArgParamPtrName());

	tmp_var_map[rt_glue.getThunkArgParamPtrName()] = allocai;
	tmp_var_map[t->getName() + "_params"] = allocai;
	builder->CreateStore(ai, allocai);

	/* skip over param pointer */
	ai++;

	/* create the rest of the arguments */
	genTypeArgs(t, &tmpB);
	if (extra_args != NULL) {
		genArgs(ai, &tmpB, extra_args->getArgsList());
	}
}

void CodeBuilder::genTypeArgs(
	const Type* t,
	llvm::IRBuilder<>		*tmpB)
{
	unsigned int			arg_c;
	const llvm::Type		*l_t;
	const ArgsList			*args;
	llvm::AllocaInst		*params_ai;

	args = t->getArgs();
	if (args == NULL)
		return;

	l_t = llvm::Type::getInt64Ty(llvm::getGlobalContext());
	arg_c = args->size();

	params_ai = tmp_var_map[t->getName() + "_params"];
	assert (params_ai != NULL && "Didn't allocate params?");

	for (unsigned int i = 0; i < arg_c; i++) {
		string				arg_name;
		string				arg_type;
		llvm::AllocaInst		*allocai;
		llvm::Value			*idx_val;
		llvm::Value			*param_elem_val;
		llvm::Value			*param_elem_ptr;

		arg_type = (args->get(i).first)->getName();
		arg_name = (args->get(i).second)->getName();

		if (symtabs.count(arg_type) != 0) {
			/* XXX TODO SUPPORT TYPE ARGS! */
			cerr << t->getName() << ": ";
			cerr << "Type args only supports scalar args" << endl;
			exit(-1);
		}

		allocai = tmpB->CreateAlloca(l_t, 0, arg_name);

		idx_val = llvm::ConstantInt::get(
			llvm::getGlobalContext(),
			llvm::APInt(32, i));

		param_elem_ptr = builder->CreateGEP(
			builder->CreateLoad(params_ai), 
			idx_val);

		param_elem_val = builder->CreateLoad(param_elem_ptr);

		builder->CreateStore(param_elem_val, allocai);
		tmp_var_map[arg_name] = allocai;
	}
}

void CodeBuilder::copyTypePassStruct(
	const Type* copy_type, llvm::Value *src, llvm::Value *dst_ptr)
{
	llvm::Value	*dst_params_ptr;
	llvm::Value	*src_params_ptr;

	/* copy contents of src into dst */
	builder->CreateStore(
		builder->CreateInsertValue(
			builder->CreateLoad(dst_ptr),
			builder->CreateExtractValue(src, 0, "cb_copy_offset"),
			0),
		dst_ptr);

	src_params_ptr = builder->CreateExtractValue(src, 1, "src_params");
	dst_params_ptr = builder->CreateExtractValue(
		builder->CreateLoad(dst_ptr), 1, "dst_params");

	emitMemcpy64(dst_params_ptr, src_params_ptr,copy_type->getNumArgs());
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
		tmp_var_map[arg_name] = allocai;
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

	bb_then = llvm::BasicBlock::Create(gctx, "then", f);
	bb_else = llvm::BasicBlock::Create(gctx, "else", f);

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


llvm::Type* CodeBuilder::getTypePassStructPtr(void)
{
	llvm::Type*	ret;

	ret = llvm::PointerType::get(typepass_struct, 0);

	return ret;
}

llvm::Type* CodeBuilder::getTypePassStruct(void)
{
	return typepass_struct;
}

void CodeBuilder::makeTypePassStruct(void)
{
	vector<const llvm::Type*>	types;

	/* diskoffset */
	types.push_back(llvm::Type::getInt64Ty(llvm::getGlobalContext()));
	/* param array */
	types.push_back(llvm::Type::getInt64PtrTy(llvm::getGlobalContext()));

	typepass_struct = llvm::StructType::get(
		llvm::getGlobalContext(), 
		types,
		"typepass");
}


llvm::AllocaInst* CodeBuilder::createTmpI64Ptr(void)
{
	llvm::AllocaInst	*ret;
	
	ret = builder->CreateAlloca(
		llvm::Type::getInt64PtrTy(llvm::getGlobalContext()),
		0,
		"__AAA" + int_to_string(tmp_c++));
	tmp_var_map[ret->getName()] = ret;

	return ret;
}

llvm::AllocaInst* CodeBuilder::getTmpAllocaInst(const std::string& s) const
{
	llvm_var_map::const_iterator	it;
	
	it = tmp_var_map.find(s);
	if (it == tmp_var_map.end())
		return NULL;

	return (*it).second;
}

llvm::AllocaInst* CodeBuilder::createTmpI64(const std::string& name)
{
	llvm::AllocaInst	*ret;
	
	ret = builder->CreateAlloca(
		llvm::Type::getInt64Ty(llvm::getGlobalContext()),
		0,
		name);
	assert (name == ret->getName());

	tmp_var_map[name] = ret;

	return ret;
}

void CodeBuilder::emitMemcpy64(
	llvm::Value* dst, llvm::Value* src, unsigned int elem_c)
{
	llvm::Function	*memcpy_f;
	const llvm::Type *Tys[] = { builder->getInt32Ty() };
	llvm::Value* args[] = {
	  	builder->CreateBitCast(
			dst, builder->getInt8PtrTy(), "dst_ptr"),
	  	builder->CreateBitCast(
			src, builder->getInt8PtrTy(), "src_ptr"),
		llvm::ConstantInt::get(
			llvm::getGlobalContext(), llvm::APInt(32, elem_c*8)),
		llvm::ConstantInt::get(builder->getInt32Ty(), 4)};

	memcpy_f =  llvm::Intrinsic::getDeclaration(
		mod, llvm::Intrinsic::memcpy, Tys, 1);
	builder->CreateCall(memcpy_f, args, args+4);
}


llvm::AllocaInst* CodeBuilder::createTmpI64(void)
{
	return createTmpI64("__BBB" + int_to_string(tmp_c++));
}

llvm::AllocaInst* CodeBuilder::createTmpTypePass(
	const Type* t, const std::string& name)
{
	assert (0 == 1);
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
