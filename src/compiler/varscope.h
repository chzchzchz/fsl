#ifndef VARSCOPE_H
#define VARSCOPE_H

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/raw_os_ostream.h>

#include <map>
#include <stdint.h>

struct TypedVar {
	TypedVar() : tv_t(NULL), tv_ai(NULL) {}
	TypedVar(const Type* in_t, llvm::AllocaInst* in_ai)
		: tv_t(in_t), tv_ai(in_ai) {}
	const Type		*tv_t;	/* NULL if primitive */
	llvm::AllocaInst	*tv_ai;
};

typedef std::map<std::string, struct TypedVar>	var_map;

class Type;

class VarScope
{
public:
	VarScope();
	VarScope(llvm::IRBuilder<>* in_b);
	virtual ~VarScope() {}

	void genTypeArgs(const Type* t);

	void loadClosureIntoThunkVars(
		llvm::Function::arg_iterator 	ai,
		llvm::IRBuilder<>		&tmpB);

	void clear(void) { vars_map.clear(); }

	void loadArgs(
		llvm::Function::arg_iterator	&ai,
		llvm::IRBuilder<>		*tmpB,
		const ArgsList			*args);
	void loadArgsFromArray(
		llvm::Value			*ai,	/* just an array ptr */
		const ArgsList			*args);

	llvm::AllocaInst* createTmpI64Ptr(void);
	llvm::AllocaInst* getTmpAllocaInst(const std::string& s) const;
	llvm::AllocaInst* createTmpI64(const std::string& name);
	llvm::AllocaInst* createTmpClosure(
		const Type* t, const std::string& name);
	/* create tmp closure from contents of parambuf */
	llvm::AllocaInst* createTmpClosure(
		const Type* t, const std::string& name,
		llvm::Value* parambuf_ai, unsigned int pb_base_idx);
	llvm::AllocaInst* createTmpClosurePtr(
		const Type* t, const std::string& name,
		llvm::Function::arg_iterator ai,
		llvm::IRBuilder<>	&tmpB);


	void print(void) const;
	unsigned int size(void) const { return vars_map.size(); }

	llvm::AllocaInst* getVar(const std::string& s) const;
	const Type* getVarType(const std::string& s) const;


private:
	var_map			vars_map;
	llvm::IRBuilder<> 	*builder;
	unsigned int		tmp_c;
};

#endif
