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

#define CLO_GET_FUNC(x, y, z)			\
llvm::Value* get##x(void) const			\
{						\
	return builder->CreateExtractValue(load(), y, z);	\
}
	CLO_GET_FUNC(Offset, RT_CLO_IDX_OFFSET, "t_offset")
	CLO_GET_FUNC(ParamBuf, RT_CLO_IDX_PARAMS, "t_params")
	CLO_GET_FUNC(Xlate, RT_CLO_IDX_XLATE, "t_virt")

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
