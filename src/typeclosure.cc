#include "code_builder.h"
#include "typeclosure.h"

extern CodeBuilder	*code_builder;

TypeClosure::TypeClosure(llvm::Function::arg_iterator arg_it)
	: clo_ptr(arg_it), builder(code_builder->getBuilder())
{
	assert (clo_ptr != NULL);
}

TypeClosure::TypeClosure(llvm::Value* in_clo)
	: clo_ptr(in_clo), builder(code_builder->getBuilder())
{
	assert (clo_ptr != NULL);
}

llvm::Value* TypeClosure::getOffset(void) const
{
	return builder->CreateExtractValue(
		load(), RT_CLO_IDX_OFFSET, "t_offset");
}

llvm::Value* TypeClosure::getParamBuf(void) const
{
	return builder->CreateExtractValue(
		load(), RT_CLO_IDX_PARAMS, "t_params");
}

llvm::Value* TypeClosure::getXlate(void) const
{
	return builder->CreateExtractValue(
		load(), RT_CLO_IDX_XLATE, "t_virt");
}

llvm::Value* TypeClosure::load(void) const
{
	return builder->CreateLoad(clo_ptr);
}

llvm::Value* TypeClosure::setXlate(llvm::Value* v)
{
	return builder->CreateInsertValue(
		load(),
		(v == NULL) ? code_builder->getNullPtrI8() : NULL,
		RT_CLO_IDX_XLATE);
}

llvm::Value* TypeClosure::setOffset(llvm::Value* v)
{
	return builder->CreateInsertValue(load(), v, RT_CLO_IDX_OFFSET);
}

llvm::Value* TypeClosure::setParamBuf(llvm::Value* v)
{
	return builder->CreateInsertValue(
		load(),
		(v == NULL) ? code_builder->getNullPtrI8() : v,
		RT_CLO_IDX_PARAMS);
}

llvm::Value* TypeClosure::setAll(
	llvm::Value* offset, llvm::Value* pb, llvm::Value* xlate)
{
	return builder->CreateInsertValue(
		builder->CreateInsertValue(
			builder->CreateInsertValue(
				load(),
				offset,
				RT_CLO_IDX_OFFSET),
			(pb == NULL) ? code_builder->getNullPtrI8() : pb,
			RT_CLO_IDX_PARAMS),
		(xlate == NULL) ? code_builder->getNullPtrI8() : xlate,
		RT_CLO_IDX_XLATE);
}


llvm::Value* TypeClosure::setOffsetXlate(llvm::Value* off, llvm::Value* xlate)
{
	return builder->CreateInsertValue(
		builder->CreateInsertValue(
			load(),
			off,
			RT_CLO_IDX_OFFSET),
		(xlate == NULL) ? code_builder->getNullPtrI8() : xlate,
		RT_CLO_IDX_XLATE);
}
