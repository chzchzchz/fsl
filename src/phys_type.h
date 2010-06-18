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
 * another type (this way we avoid possible recursion in the compile stage) 
 * thunk functions take the form
 * __thunk_xxx(TYPENAME, arg1, arg2, ...)
 *
 * NOTE: for single argument types, we can directly replace these..
 */
class PhysTypeThunk : public PhysicalType
{
public:
	PhysTypeThunk(const Type* t, ExprList* in_exprs = NULL) 
	: PhysicalType(std::string("thunk_") + t->getName()),
	  type(t), exprs(in_exprs)
	{
		if (exprs == NULL) 
			exprs = new ExprList();
	}

	virtual ~PhysTypeThunk() 
	{
		if (exprs != NULL) delete exprs;
	}

	const Type* getType(void) const { return type; }

	virtual Expr* getBytes(void) const
	{
		assert (exprs != NULL);
		return new FCall(new Id("__thunk_bytes"), exprs);
	}

	virtual Expr* getBits(void) const
	{
		assert (exprs != NULL);
		return new FCall(new Id("__thunk_bits"), exprs);
	}

	PhysicalType* copy(void) const
	{
		return new PhysTypeThunk(type, new ExprList(*exprs));
	}

	bool setArgs(ExprList* args)
	{
		if (args == NULL) {
			if (type->getArgs()->size() != 0)
				return false;
			
			if (exprs != NULL) delete exprs;

			exprs = new ExprList();
			return true;
		}

		if (args->size() != type->getArgs()->size()) {
			/* thunk arg mismatch */
			return false;
		}

		if (exprs != NULL) delete exprs;

		exprs = args;
		exprs->insert(exprs->begin(), new Id(type->getName()));

		return true;
	}

private:
	const Type	*type;
	ExprList	*exprs;
};

class PhysTypeArray : public PhysicalType
{
public:
	PhysTypeArray(PhysicalType* in_base, Expr* in_length)
	:	PhysicalType(in_base->getName() + "[]"),
		base(in_base),
		len(in_length)
	{
		assert (base != NULL);
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

class PhysTypeUser : public PhysicalType
{
public:
	PhysTypeUser(const Type* in_t, PhysicalType* in_resolved)
	: 	PhysicalType(in_t->getName()),
		t(in_t),
		resolved(in_resolved)
	{
		assert (t != NULL);
		assert (resolved != NULL);
	}

	virtual ~PhysTypeUser()
	{
		delete resolved;
	}

	virtual Expr* getBytes(void) const { return resolved->getBytes(); }
	virtual Expr* getBits(void) const { return resolved->getBits(); }

	PhysicalType* copy(void) const
	{
		return new PhysTypeUser(t, resolved->copy());
	}

private:
	const Type*		t;
	PhysicalType*		resolved;
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
