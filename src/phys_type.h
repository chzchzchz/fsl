/**
 * physical types represent an extent of bits
 * In order to resolve a type field, we need both a physical type and an offset.
 */
#ifndef PHYS_TYPE_H
#define PHYS_TYPE_H

#include "type.h"
#include <string>
#include <assert.h>
#include <stdint.h>
#include "collection.h"
#include "util.h"

class PhysicalType
{
public:
	PhysicalType(const std::string& in_name) : name(in_name) { }

	/* resolve as much as possible.. must be able to compute this 
	 * from the containing scope! */
	virtual Expr* getBytes(void) const = 0;
	virtual Expr* getBits(void) const 
	{ 
		return new AOPLShift(getBytes(), new Number(3));
	}

	const std::string& getName(void) const { return name; }
	void setName(const std::string& newname) { name = newname; }

	bool operator ==(const PhysicalType& t) const
		{ return  (name == t.name); }
	bool operator <(const PhysicalType& t) const
		{ return (name < t.name); }

	virtual PhysicalType* copy(void) const = 0;

	virtual const llvm::Type* getLLVMType(void) const { return NULL; }

private:
	std::string name;
};


class PhysTypeUnion : public PhysicalType, public PtrList<PhysicalType>
{
public:
	PhysTypeUnion(const std::string& name) : PhysicalType(name)  {}
	PhysTypeUnion(const PhysTypeUnion& ptu)
	: PhysicalType(ptu.getName()),
	  PtrList<PhysicalType>(ptu) {}

	virtual Expr* getBytes(void) const 
	{
		const_iterator	it;
		ExprList	*exprs;

		it = begin();
		if (it == end()) {
			return new Number(0);
		}

		exprs = new ExprList();
		exprs->add((*it)->getBytes());

		it++;
		for (; it != end(); it++) {
			exprs->add((*it)->getBytes());
		}

		return new FCall(
			new Id("__max" + int_to_string(exprs->size())), 
			exprs);

	}

	virtual Expr* getBits(void) const
	{
		const_iterator	it;
		ExprList	*exprs;

		it = begin();
		if (it == end()) {
			return new Number(0);
		}

		exprs = new ExprList();
		exprs->add((*it)->getBits());

		it++;
		for (; it != end(); it++) {
			exprs->add((*it)->getBits());
		}

		return new FCall(
			new Id("__max" + int_to_string(exprs->size())), 
			exprs);

	}

	PhysicalType* copy(void) const
	{
		return new PhysTypeUnion(*this);
	}

};

class PhysTypeEmpty : public PhysicalType
{
public:
	PhysTypeEmpty() : PhysicalType("nop") {}
	virtual Expr* getBytes(void) const { return new Number(0); }
	PhysicalType* copy(void) const { return new PhysTypeEmpty(); }
};

template <typename T>
class PhysTypePrimitive : public PhysicalType
{
public:
	PhysTypePrimitive(const std::string& in_name) : PhysicalType(in_name) {}
	virtual Expr* getBytes(void) const { return new Number(sizeof(T)); }
	PhysicalType* copy(void) const
	{
		return new PhysTypePrimitive<T>(getName());
	}
private:
};

class I16 : public PhysTypePrimitive<int16_t>
{
public:
	I16() : PhysTypePrimitive<int16_t>("i16") {}
	virtual ~I16() {}
	virtual const llvm::Type* getLLVMType(void) const;
private:
};

class I32 : public PhysTypePrimitive<int32_t>
{
public:
	I32 () : PhysTypePrimitive<int32_t>("i32") {}
	virtual ~I32() {}
	virtual const llvm::Type* getLLVMType(void) const;
private:
};

class I64 : public PhysTypePrimitive<int64_t>
{
public:
	I64 () : PhysTypePrimitive<int64_t>("i64") {}
	virtual ~I64() {}
	virtual const llvm::Type* getLLVMType(void) const;
private:
};



class U64 : public PhysTypePrimitive<uint64_t>
{
public:
	U64 () : PhysTypePrimitive<uint64_t>("u64") {}
	virtual ~U64() {}
	virtual const llvm::Type* getLLVMType(void) const;
private:
};

class U32 : public PhysTypePrimitive<uint32_t>
{
public:
	U32 () : PhysTypePrimitive<uint32_t>("u32") {}
	virtual ~U32() {}
	virtual const llvm::Type* getLLVMType(void) const;
private:
};

class U16 : public PhysTypePrimitive<uint16_t>
{
public:
	U16() : PhysTypePrimitive<uint16_t>("u16") {}
	virtual ~U16() {}
	virtual const llvm::Type* getLLVMType(void) const;
private:
};


class U8 : public PhysTypePrimitive<uint8_t>
{
public:
	U8() : PhysTypePrimitive<uint8_t>("u8") {}
	virtual ~U8() {}
	virtual const llvm::Type* getLLVMType(void) const;
private:
};


template <int N>
class PhysTypeBits : public PhysicalType
{
public:
	PhysTypeBits(const std::string& in_name) : PhysicalType(in_name) {}
	virtual Expr* getBytes(void) const { return new Number((N+7)/8); }
	virtual Expr* getBits(void) const { return new Number(N); }
	PhysicalType* copy(void) const
	{ return new PhysTypeBits<N>(getName()); }
};

class U12 : public PhysTypeBits<12>
{
public:
	U12() : PhysTypeBits<12>("u12") {}
	virtual ~U12() {}
private:

};

class U1 : public PhysTypeBits<1>
{
public:
	U1() : PhysTypeBits<1>("u1") {}
	virtual ~U1() {}
	virtual const llvm::Type* getLLVMType(void) const;
private:
};



#endif
