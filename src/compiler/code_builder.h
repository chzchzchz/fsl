#ifndef CODEBUILDER_H
#define CODEBUILDER_H

#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Support/raw_os_ostream.h>

#include "args.h"
#include <map>
#include <stdint.h>

typedef std::map<std::string, llvm::AllocaInst*>	llvm_var_map;

class VarScope;

class CodeBuilder
{
public:
	CodeBuilder(const char* mod_name);
	virtual ~CodeBuilder(void);

	llvm::Module* getModule(void) { return mod; }
	llvm::IRBuilder<>* getBuilder(void) { return builder; }

	void createGlobal(const char* str, uint64_t v, bool is_const);
#define createGlobalConst(x, y) createGlobal(x, y, true)
#define createGlobalMutable(x, y) createGlobal(x, y, false)

	/* number of args = # of 64-bit ents */
	llvm::Function* genProto(const std::string& name, uint64_t num_args);
	llvm::Function* genProto(
		const std::string& 	name,
		const llvm::Type	*ret_type,
		const llvm::Type	*t1);
	llvm::Function* genProto(
		const std::string& 	name,
		const llvm::Type	*ret_type,
		const llvm::Type *t1, const llvm::Type *t2);
	llvm::Function* genProto(
		const std::string& 	name,
		const llvm::Type	*ret_type,
		const llvm::Type *t1, const llvm::Type *t2,
		const llvm::Type *t3);
	llvm::Function* genProtoV(
		const std::string& name,
		const llvm::Type* ret_type,
		const std::vector<const llvm::Type*>& args);
	void genCode(
		const class Type* t,
		const std::string& name,
		const class Expr* raw_expr,
		const class ArgsList* extra_args = NULL);

	void genCodeEmpty(const std::string& name);

	void genCodeCond(
		const Type		*t,
		const std::string&	name,
		const class CondExpr	*cond_expr,
		const Expr		*true_expr,
		const Expr		*false_expr);

	void genThunkProto(const std::string& name);
	void genThunkProto(
		const std::string	&name,
		const llvm::Type	*ret_type);

	void write(std::ostream& os);
	void write(std::string& os);

	unsigned int getTmpVarCount(void) const;
	llvm::AllocaInst* getTmpAllocaInst(const std::string& s) const;
	void printTmpVars(void) const;
	llvm::AllocaInst* createTmpI64(const std::string& name);
	llvm::AllocaInst* createTmpI64(void);
	llvm::AllocaInst* createPrivateTmpI64Array(
		unsigned int num_elems,
		const std::string& name);

	llvm::Value* getNullPtrI64(void);
	llvm::Value* getNullPtrI8(void);

	llvm::AllocaInst* createPrivateClosure(const Type* t);

	llvm::AllocaInst* createTmpClosure(
		const Type* t,
		const std::string& name = "");

	llvm::AllocaInst* createTmpI64Ptr(void);
	llvm::GlobalVariable* getGlobalVar(const std::string& varname) const;
	llvm::Value* loadPtr(llvm::Value* ptr, unsigned int idx);

	void setDebug(bool b) { debug_output = b; }


	llvm::Type* getClosureTy(void) { return closure_struct; }
	llvm::Type* getClosureTyPtr(void);
	llvm::Type* getVirtTyPtr(void);
	const llvm::Type* getI64TyPtr(void);

	void copyClosure(const Type* t,
		llvm::Value *src, llvm::Value *dst_ptr);

	void genThunkHeaderArgs(
		llvm::Function* f, const Type* t,
		const ArgsList* e_args = NULL);

	void emitMemcpy64(
		llvm::Value* dst, llvm::Value* src,
		unsigned int elems);

	void loadArgsFromParamBuf(llvm::Value* ai, const ArgsList* args);
	void loadArgsFromParamBuf(
		llvm::Function::arg_iterator	arg_it,
		const ArgsList			*args);

	const VarScope* getVarScope() const { return vscope; }

	void storeClosureIntoParamBuf(
		llvm::AllocaInst* params_out_ptr, unsigned int pb_idx,
		const Type* t, llvm::Value* clo);

	int storeExprIntoParamBuf(
		const arg_elem& cur_arg,
		const Expr	*expr,
		llvm::AllocaInst* params_out_ptr,
		unsigned int pb_base_idx);

	int storeExprListIntoParamBuf(
		class EvalCtx		*ectx,
		const ArgsList		*args,
		const class ExprList	*expr,
		llvm::AllocaInst* params_out_ptr);
private:
	void makeClosureTy(void);


	CodeBuilder() {}
	llvm::Module		*mod;
	llvm::IRBuilder<> 	*builder;
	llvm::Type		*closure_struct;
	VarScope		*vscope;
	bool			debug_output;
	unsigned int		tmp_c;
};

#endif
