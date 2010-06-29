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

#if 0
class PhysTypeDisk : public PhysicalType
{
public:
	PhysTypeDisk(void) : PhysicalType(disk) {}

	virtual ~PhysTypeDisk() {}

	virtual Expr* getBits(void) const
	{
		return new FCall(new Id("__disk_bits"), new ExprList());
	}

	virtual Expr* getBytes(void) const
	{
		return new FCall(new Id("__disk_bits"), new ExprList());
	}

	virtual PhysicalType* copy(void) const { return new PhysTypeDisk(); }
};
#endif

class PhysTypeFunc : public PhysicalType
{
public:
	PhysTypeFunc(FCall* fc) 
	: PhysicalType(std::string("func_") + fc->getName()),
	  func(fc)
	{
		assert (fc != NULL);
	}

	virtual ~PhysTypeFunc() { delete func; }

	Expr* getBytes(void) const
	{
		return new FCall(
			new Id(func->getName() + "_bytes"), 
			(func->getExprs())->copy());
	}

	Expr* getBits(void) const
	{
		return new FCall(
			new Id(func->getName() + "_bits"),
			(func->getExprs())->copy());
	}

	virtual PhysicalType* copy(void) const { 
		return new PhysTypeFunc(func->copy());
	}

private:
	FCall	*func;
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

		return new FCall(new Id("__max"), exprs);
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

		return new FCall(new Id("__max"), exprs);

	}

	PhysicalType* copy(void) const
	{
		return new PhysTypeUnion(*this);
	}

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
		Expr		*sum, *simp_sum;

		it = begin();
		if (it == end()) {
			return new Number(0);
		}

		sum = (*it)->getBytes();
		it++;
		for (; it != end(); it++) {
			sum = new AOPAdd(sum, (*it)->getBytes());
		}

		simp_sum = sum->simplify();
		delete sum;
		return simp_sum;
	}

	virtual Expr* getBits(void) const
	{
		const_iterator	it;
		Expr		*sum, *simp_sum;

		it = begin();
		if (it == end()) {
			return new Number(0);
		}

		sum = (*it)->getBits();
		it++;
		for (; it != end(); it++) {
			sum = new AOPAdd(sum, (*it)->getBits());
		}

		simp_sum = sum->simplify();
		delete sum;
		return simp_sum;
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
		return new FCall(new Id(
			std::string("__thunk_")	
			+ type->getName() + std::string("_bytes")), 
			exprs->simplify());
	}

	virtual Expr* getBits(void) const
	{
		assert (exprs != NULL);
		return new FCall(new Id(
			std::string("__thunk_")	
			+ type->getName() + std::string("_bytes")), 
			exprs->simplify());
	}

	PhysicalType* copy(void) const
	{
		return new PhysTypeThunk(type, new ExprList(*exprs));
	}

	bool setArgs(ExprList* args)
	{
		const ArgsList*	type_args;

		type_args = type->getArgs();

		if (args == NULL) {
			if (type_args != NULL)
				return false;
			
			if (exprs != NULL) delete exprs;

			exprs = new ExprList();
			return true;
		}

		if (type_args == NULL || 
		    args->size() != type_args->size()) {
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

	Expr* getBytes(Expr* idx) const
	{
		/* XXX needs check to ensure we don't go out of bounds */
		return new AOPMul(base->getBytes(), idx->simplify());
	}

	Expr* getBits(Expr* idx) const
	{
		/* XXX Needs check to ensure we don't go out of bounds */
		return new AOPMul(base->getBits(), idx->simplify());
	}

	const PhysicalType* getBase() const { return base; }

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

	const Type* getType(void) const 
	{
		return t;
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

class I64 : public PhysTypePrimitive<int64_t>
{
public:
	I64 () : PhysTypePrimitive<int64_t>("i64") {}
	virtual ~I64() {}
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


class PhysTypeCond : public PhysicalType
{
public:
	PhysTypeCond(
		const std::string& in_cond_name,
		Expr* in_e1, Expr* in_e2,
		PhysicalType* in_true, PhysicalType* in_false)
	: PhysicalType("cond"),
	  cond_name(in_cond_name),
	  e1(in_e1),
	  e2(in_e2),
	  is_true(in_true),
	  is_false(in_false)
	{
		assert (e1 != NULL);
		assert (e2 != NULL);
		assert (in_true != NULL);

		if (is_false == NULL) {
			is_false = new PhysTypeEmpty();
		}
	}

	virtual ~PhysTypeCond() 
	{
		delete e1;
		delete e2;
		delete is_true;
		delete is_false;
	}

	virtual Expr* getBytes(void) const
	{
		ExprList*	exprs = new ExprList();

		exprs->add(e1->simplify());
		exprs->add(e2->simplify());
		exprs->add(is_true->getBytes());
		exprs->add(is_false->getBytes());

		return new FCall(new Id(cond_name), exprs);
	}

	virtual Expr* getBits(void) const
	{
		ExprList*	exprs = new ExprList();

		exprs->add(e1->simplify());
		exprs->add(e2->simplify());
		exprs->add(is_true->getBits());
		exprs->add(is_false->getBits());

		return new FCall(new Id(cond_name), exprs);;
	}

protected:
	const std::string	cond_name;
	Expr			*e1;
	Expr			*e2;
	PhysicalType		*is_true;
	PhysicalType		*is_false;
};

class PhysTypeCondEQ : public PhysTypeCond
{
public:
	PhysTypeCondEQ(Expr* e1, Expr* e2, PhysicalType* t, PhysicalType* f)
	: PhysTypeCond("cond_eq", e1, e2, t, f) {}
	virtual ~PhysTypeCondEQ() {}
	virtual PhysTypeCondEQ* copy() const { 
		return new PhysTypeCondEQ(
			e1->copy(), e2->copy(), 
			is_true->copy(),
			(is_false == NULL) ? NULL : is_false->copy());
	}
};


class PhysTypeCondNE : public PhysTypeCond
{
public:
	PhysTypeCondNE(Expr* e1, Expr* e2, PhysicalType* t, PhysicalType* f)
	: PhysTypeCond("cond_ne", e1, e2, t, f) {}
	virtual ~PhysTypeCondNE() {}

	virtual PhysTypeCondNE* copy() const { 
		return new PhysTypeCondNE(
			e1->copy(), e2->copy(), 
			is_true->copy(),
			(is_false == NULL) ? NULL : is_false->copy());
	}
};

class PhysTypeCondLE : public PhysTypeCond
{
public:
	PhysTypeCondLE(Expr* e1, Expr* e2, PhysicalType* t, PhysicalType* f)
	: PhysTypeCond("cond_le", e1, e2, t, f) {}
	virtual ~PhysTypeCondLE() {}
	virtual PhysTypeCondLE* copy() const { 
		return new PhysTypeCondLE(
			e1->copy(), e2->copy(), 
			is_true->copy(),
			(is_false == NULL) ? NULL : is_false->copy());
	}
};

class PhysTypeCondGE : public PhysTypeCond
{
public:
	PhysTypeCondGE(Expr* e1, Expr* e2, PhysicalType* t, PhysicalType* f)
	: PhysTypeCond("cond_ge", e1, e2, t, f) {}
	virtual ~PhysTypeCondGE() {}
	virtual PhysTypeCondGE* copy() const { 
		return new PhysTypeCondGE(
			e1->copy(), e2->copy(), 
			is_true->copy(),
			(is_false == NULL) ? NULL : is_false->copy());
	}
};

class PhysTypeCondLT : public PhysTypeCond
{
public:
	PhysTypeCondLT(Expr* e1, Expr* e2, PhysicalType* t, PhysicalType* f)
	: PhysTypeCond("cond_lt", e1, e2, t, f) {}
	virtual ~PhysTypeCondLT() {}
	virtual PhysTypeCondLT* copy() const { 
		return new PhysTypeCondLT(
			e1->copy(), e2->copy(), 
			is_true->copy(),
			(is_false == NULL) ? NULL : is_false->copy());
	}
};

class PhysTypeCondGT : public PhysTypeCond
{
public:
	PhysTypeCondGT(Expr* e1, Expr* e2, PhysicalType* t, PhysicalType* f)
	: PhysTypeCond("cond_gt", e1, e2, t, f) {}
	virtual ~PhysTypeCondGT() {}
	virtual PhysTypeCondGT* copy() const { 
		return new PhysTypeCondGT(
			e1->copy(), e2->copy(), 
			is_true->copy(),
			(is_false == NULL) ? NULL : is_false->copy());
	}
};

static inline  PhysicalType* resolve_by_id(const ptype_map& tm, const Id* id);
static inline PhysicalType* resolve_by_id(const ptype_map& tm, const Id* id)
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



#endif
