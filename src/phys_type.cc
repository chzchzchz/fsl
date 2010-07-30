#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/IRBuilder.h>

#include "phys_type.h"

#if 0
const llvm::Type* U1::getLLVMType(void) const
{
	return llvm::Type::getInt1Ty(llvm::getGlobalContext());
}

const llvm::Type* U8::getLLVMType(void) const
{
	return llvm::Type::getInt8Ty(llvm::getGlobalContext());
}

const llvm::Type* I16::getLLVMType(void) const
{
	return llvm::Type::getInt16Ty(llvm::getGlobalContext());
}

const llvm::Type* I32::getLLVMType(void) const
{
	return llvm::Type::getInt32Ty(llvm::getGlobalContext());
}

const llvm::Type* I64::getLLVMType(void) const
{
	return llvm::Type::getInt64Ty(llvm::getGlobalContext());
}


const llvm::Type* U16::getLLVMType(void) const
{
	return llvm::Type::getInt16Ty(llvm::getGlobalContext());
}

const llvm::Type* U32::getLLVMType(void) const
{
	return llvm::Type::getInt32Ty(llvm::getGlobalContext());
}

const llvm::Type* U64::getLLVMType(void) const
{
	return llvm::Type::getInt64Ty(llvm::getGlobalContext());
}
#endif

const llvm::Type* U1::getLLVMType(void) const
{
	return llvm::Type::getInt64Ty(llvm::getGlobalContext());
}

const llvm::Type* U8::getLLVMType(void) const
{
	return llvm::Type::getInt64Ty(llvm::getGlobalContext());
}

const llvm::Type* I16::getLLVMType(void) const
{
	return llvm::Type::getInt64Ty(llvm::getGlobalContext());
}

const llvm::Type* I32::getLLVMType(void) const
{
	return llvm::Type::getInt64Ty(llvm::getGlobalContext());
}

const llvm::Type* I64::getLLVMType(void) const
{
	return llvm::Type::getInt64Ty(llvm::getGlobalContext());
}


const llvm::Type* U16::getLLVMType(void) const
{
	return llvm::Type::getInt64Ty(llvm::getGlobalContext());
}

const llvm::Type* U32::getLLVMType(void) const
{
	return llvm::Type::getInt64Ty(llvm::getGlobalContext());
}

const llvm::Type* U64::getLLVMType(void) const
{
	return llvm::Type::getInt64Ty(llvm::getGlobalContext());
}



