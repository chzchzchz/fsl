#ifndef PHYS_TYPE_H
#define PHYS_TYPE_H

#include <stdint.h>

class PhysicalType
{
public:
	PhysicalType(const string& in_name) : string(name) { }

	/* resolve as much as possible.. must be able to compute this 
	 * from the containing scope! */
	virtual Expr* getBytes(void) const = 0;
	virtual Expr* getBits(void) const 
	{ 
		return new AOPLShift(getBytes(), 3);
	}

	const std::string& getName(void) const { return name; }

	bool operator ==(const PhysicalType& t) const
		{ return  (name == t.name); }
	bool operator <(const PhysicalType& t) const
		{ return (name < t.name); }
private:
	std::string name;
};

template <typename T>
class PhysTypePrimitive<T> : public PhysicalType
{
public:
	PhysTypePrimitive<T>(const string& in_name) : PhysicalType(in_name) {}
	virtual Expr* getBytes(void) const { return new Number(sizeof(T)); }
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


class U16 : public PhysTypePrimitive<uint16_t>
{
public:
	U16() : PhysTypePrimitive<uint16_t>("u16") {}
	virtual ~U16() {}
private:
};

class U32 : public PhysTypePrimitive<uint32_t>
{
public:
	U32 () : PhysTypePrimitive<uint32_t>("u32") {}
	virtual ~U32() {}
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
class PhysTypeBits<N> : public PhysicalType
{
	PhysTypePrimitive<T>(const string& in_name) : PhysicalType(in_name) {}
	virtual Expr* getBytes(void) const { return new Number((N+7)/8); }
	virtual Expr* getBits(void) const { return new Number(N); }
}

class U12 : public PhysTypeBits<12>
{
public:
	U12() : PhysTypePrimitive<12>("u12") {}
	virtual ~U12() {}
private:

};

class U1 : public PhysTypeBits<1>
{
public:
	U1() : PhysTypePrimitive<1>("u1") {}
	virtual ~U1() {}
private:


}

#endif
