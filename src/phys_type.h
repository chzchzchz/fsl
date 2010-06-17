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
private:
	std::string name;
};

/**
 * collection of multiple types wrapped into a single aggregate.
 */
class PhysTypeAggregate : public PhysicalType, public PtrList<PhysicalType>
{
public:
	PhysTypeAggregate(const std::string& name) : PhysicalType(name)  {}
	PhysTypeAggregate(const PhysTypeAggregate& pta)
	: PhysicalType(pta.getName()),
	  PtrList<PhysicalType>(pta) {}

	virtual Expr* getBytes(void) const 
	{
		const_iterator	it;
		Expr		*sum;

		it = begin();
		if (it == end()) {
			return new Number(0);
		}

		sum = (*it)->getBytes();
		it++;
		for (; it != end(); it++) {
			sum = new AOPAdd(sum, (*it)->getBytes());
		}

		return sum;
	}

	virtual Expr* getBits(void) const
	{
		const_iterator	it;
		Expr		*sum;

		it = begin();
		if (it == end()) {
			return new Number(0);
		}

		sum = (*it)->getBits();
		it++;
		for (; it != end(); it++) {
			sum = new AOPAdd(sum, (*it)->getBits());
		}

		return sum;
	}

	PhysicalType* copy(void) const
	{
		return new PhysTypeAggregate(*this);
	}
};

/* this should be used for evaluated parameterized type declarations within
 * another type (this way we avoid possible recursion in the compile stage) */
class PhysTypeThunk : public PhysicalType
{
public:
	PhysTypeThunk(
		const std::string& in_type,
		ExprList* in_exprs) 
	: PhysicalType("thunk_" + in_type),
	  type(in_type)
	{
		if (in_exprs != NULL) {
			exprs = in_exprs;
			exprs->insert(exprs->begin(), new Id(type));
		} else {
			exprs = new ExprList();
			exprs->add(new Id(type));
		}
	}

	virtual ~PhysTypeThunk() 
	{
		delete exprs;
	}

	virtual Expr* getBytes(void) const
	{
		return new FCall(new Id("__thunk_bytes"), exprs);
	}

	virtual Expr* getBits(void) const
	{
		return new FCall(new Id("__thunk_bits"), exprs);
	}

private:
	std::string 	type;
	ExprList*	exprs;
	
};

class PhysTypeArray : public PhysicalType
{
public:
	PhysTypeArray(PhysicalType* base, Expr* in_length)
	:	PhysicalType(base->getName() + "[]"),
		len(in_length)
	{
		assert (len != NULL);
	}

	virtual ~PhysTypeArray()
	{
		delete base;
		delete len;
	}

	virtual Expr* getBytes(void) const
	{
		return new AOPMul(len->copy(), base->getBytes());
	}

	virtual Expr* getBits(void) const
	{
		return new AOPMul(len->copy(), base->getBits());
	}

	PhysicalType* copy(void) const 
	{
		return new PhysTypeArray(base->copy(), len->copy());
	}

private:
	PhysicalType	*base;
	Expr		*len;
};

class PhysTypeEmpty : public PhysicalType
{
public:
	PhysTypeEmpty() : PhysicalType("nop") {}
	virtual Expr* getBytes(void) const { return new Number(0); }
	PhysicalType* copy(void) const { return new PhysTypeEmpty(); }
};

#if 0
class PhysTypeUser : public PhysicalType
{
public:
	PhysTypeUser(const Type* in_t, uint32_t in_cond_bmp)
	: 	PhysicalType(in_t->getName()),
		t(in_t),
		cond_bmp(in_cond_bmp) {}

	virtual ~PhysTypeUser() {}

	virtual Expr* getBytes(void) const { return t->getBytes(cond_bmp); }
	virtual Expr* getBits(void) const { return t->getBits(cond_bmp); }

	void setConditionBits(uint32_t cond_bits) { cond_bmp = cond_bits; }
	uint32_t getConditionBits() const { return cond_bmp; }

	PhysicalType* copy(void) const { return new PhysTypeUser(t, cond_bmp); }

private:
	const Type*		t;
	uint32_t		cond_bmp;
};
#endif

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
private:
};

class I32 : public PhysTypePrimitive<int32_t>
{
public:
	I32 () : PhysTypePrimitive<int32_t>("i32") {}
	virtual ~I32() {}
private:
};


class U64 : public PhysTypePrimitive<uint64_t>
{
public:
	U64 () : PhysTypePrimitive<uint64_t>("u64") {}
	virtual ~U64() {}
private:
};

class U32 : public PhysTypePrimitive<uint32_t>
{
public:
	U32 () : PhysTypePrimitive<uint32_t>("u32") {}
	virtual ~U32() {}
private:
};

class U16 : public PhysTypePrimitive<uint16_t>
{
public:
	U16() : PhysTypePrimitive<uint16_t>("u16") {}
	virtual ~U16() {}
private:
};


class U8 : public PhysTypePrimitive<uint8_t>
{
public:
	U8() : PhysTypePrimitive<uint8_t>("u8") {}
	virtual ~U8() {}
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
private:
};

#endif
