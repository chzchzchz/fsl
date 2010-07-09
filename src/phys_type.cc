#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/IRBuilder.h>

#include "phys_type.h"

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


const llvm::Type* PhysTypeUser::getLLVMType(void) const
{
	return llvm::Type::getInt64Ty(llvm::getGlobalContext());
}

PhysicalType* resolve_by_id(const ptype_map& tm, const Id* id)
{
	const std::string&		id_name(id->getName());
	ptype_map::const_iterator	it;

	/* try to resovle type directly */
	it = tm.find(id_name);
	if (it != tm.end()) {
		return ((*it).second)->copy();
	}

	/* type does not resolve directly, may have a thunk available */
	it = tm.find(std::string("thunk_") + id_name);
	if (it == tm.end()) {
		std::cerr << "Could not resolve type: ";
		std::cerr << id_name << std::endl;
		return NULL;
	}	

	return ((*it).second)->copy();
}

/* return true if PT has a user-defined base type */
bool isPTaUserType(const PhysicalType* pt)
{
	const PhysTypeArray	*pta;

	if (dynamic_cast<const PhysTypeUser*>(pt) != NULL)
		return true;
	if (dynamic_cast<const PhysTypeThunk*>(pt) != NULL) 	
		return true;

	pta = dynamic_cast<const PhysTypeArray*>(pt);
	if (pta == NULL)
		return false;

	return isPTaUserType(pta->getBase());
}

/* convert a user-defined physical type into the proper type, if possible */
const Type* PT2Type(const PhysicalType* pt)
{
	const PhysTypeUser	*ptu;
	const PhysTypeThunk	*ptthunk;

	ptu = dynamic_cast<const PhysTypeUser*>(pt);
	if (ptu != NULL) {
		return ptu->getType();
	}

	ptthunk = dynamic_cast<const PhysTypeThunk*>(pt);
	if (ptthunk != NULL)
		return ptthunk->getType();

	return NULL;
}


