#ifndef CODEBUILDER_H
#define CODEBUILDER_H

#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Support/raw_os_ostream.h>

#include <map>
#include <stdint.h>

typedef std::map<std::string, llvm::AllocaInst*>	llvm_var_map;

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
	void genProto(const std::string& name, uint64_t num_args);
	void genCode(
		const class Type* t,
		const std::string& name, 
		const class Expr* raw_expr,
		const class FuncArgs* extra_args = NULL);

	void genCodeCond(
		const Type		*t,
		const std::string&	name,
		const class CondExpr	*cond_expr,
		const Expr		*true_expr,
		const Expr		*false_expr);

	void write(std::ostream& os);

	unsigned int getThunkVarCount(void) const { return thunk_var_map.size(); }
	llvm::AllocaInst* getThunkAllocInst(const std::string& s) const 
	{
		llvm_var_map::const_iterator	it;
		
		it = thunk_var_map.find(s);
		if (it == thunk_var_map.end())
			return NULL;

		return (*it).second;
	}
private:

	void genHeaderArgs(
		llvm::Function* f, const Type* t, 
		const FuncArgs* e_args = NULL);

	void genArgs(
		llvm::Function::arg_iterator	&ai,
		llvm::IRBuilder<>		*tmpB,
		const class ArgsList		*args);

	CodeBuilder() {}
	llvm::Module		*mod;
	llvm::IRBuilder<> 	*builder;
	llvm_var_map		thunk_var_map;
};

#endif
