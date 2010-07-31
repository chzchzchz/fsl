#ifndef CODEBUILDER_H
#define CODEBUILDER_H

#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Support/raw_os_ostream.h>

#include <stdint.h>

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
		const class Expr* raw_expr);

	void genCodeCond(
		const Type		*t,
		const std::string&	name,
		const class CondExpr	*cond_expr,
		const Expr		*true_expr,
		const Expr		*false_expr);

	void write(std::ostream& os);
private:

	void genHeaderArgs(const Type* t, llvm::Function* f);

	CodeBuilder() {}
	llvm::Module		*mod;
	llvm::IRBuilder<> 	*builder;
};

#endif
