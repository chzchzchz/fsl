#ifndef TYPECLOSURE_H
#define TYPECLOSURE_H

#include <assert.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Module.h>
#include <llvm/Support/IRBuilder.h>

#define RT_CLO_IDX_OFFSET			0
#define RT_CLO_IDX_PARAMS			1
#define RT_CLO_IDX_XLATE			2
#define RT_CLO_ELEMS				3

class TypeClosure
{
public:
	TypeClosure(llvm::Value* in_clo);
	TypeClosure(llvm::Function::arg_iterator arg_it);

	llvm::Value* getOffset(void) const;
	llvm::Value* getParamBuf(void) const;
	llvm::Value* getXlate(void) const;
	llvm::Value* setOffset(llvm::Value* v);
	llvm::Value* setParamBuf(llvm::Value* v);
	llvm::Value* setXlate(llvm::Value* v);
	llvm::Value* setAll(
		llvm::Value* offset, llvm::Value* pb, llvm::Value* xlate);
	llvm::Value* setOffsetXlate(llvm::Value* off, llvm::Value* xlate);
	llvm::Value* getPtr(void) const { return clo_ptr; }
	virtual ~TypeClosure() {}
private:
	llvm::Value*	load() const;
	TypeClosure() {}
	llvm::Value		*clo_ptr;
	llvm::IRBuilder<>	*builder;
};

#endif
