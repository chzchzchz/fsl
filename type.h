#ifndef TYPE_H
#define TYPE_H

#include <iostream>
#include <list>
#include <assert.h>
#include "collection.h"

#include "expr.h"

class TypeStmt
{
public:
	virtual ~TypeStmt() {}
	virtual void print(std::ostream& out) const = 0;
	unsigned int getLineNo() const { return lineno; }
	void setLineNo(unsigned int l) { lineno = l; }
protected:
	TypeStmt() {}
private:
	unsigned int lineno;
};

class TypeBB : public PtrList<TypeStmt>
{
public:
	TypeBB() {}
	~TypeBB() {}
private:
};

/* declaration */
class TypeDecl : public TypeStmt
{
public:
	TypeDecl(Id* in_type, Id* in_name)
	:	type(in_type),
		name(in_name),
		array(NULL)
	{
		assert (type != NULL);
		assert (name != NULL);
	}

	TypeDecl(Id* in_type, IdArray* in_array)
	:	type(in_type),
		name(NULL),
		array(in_array)
	{
		assert (type != NULL);
		assert (array != NULL);
	}
	virtual ~TypeDecl()
	{
		delete type;
		if (name != NULL) delete name;
		if (array != NULL) delete array;
	}

	void print(std::ostream& out) const 
	{
		out << "DECL: "; 
		if (array != NULL)	array->print(out);
		else			name->print(out);
		out << " -> ";
		type->print(out);
	}
private:
	Id*		type;
	Id*		name;
	IdArray*	array;
};

class TypeParamDecl : public TypeStmt
{
public:
	TypeParamDecl(FCall* in_type, Id* in_name)
	: name(in_name), array(NULL), type(in_type) 
	{
		assert (name != NULL);
		assert (type != NULL);
	}

	TypeParamDecl(FCall* in_type, IdArray* in_array) 
	: name(NULL), array(in_array), type(in_type)
	{
		assert (array != NULL);
		assert (type != NULL);
	}

	virtual ~TypeParamDecl() 
	{
		if (name != NULL) delete name;
		if (array != NULL) delete array;
		delete type;
	}

	void print(std::ostream& out) const 
	{
		out << "PDECL: "; 
		if (array != NULL)	array->print(out);
		else			name->print(out);
		out << " -> ";
		type->print(out);
	}
private:
	Id*		name;
	IdArray*	array;
	FCall*		type;
};


class TypeCond: public TypeStmt
{
public:
	TypeCond(CondExpr* ce, TypeStmt* taken) : TypeStmt() {}
	TypeCond(CondExpr* ce, TypeStmt* taken, TypeStmt* nottaken) 
		: TypeStmt() 
	{
		
	}

	void print(std::ostream& out) const { out << "TYPECOND"; }
};

class TypePreamble : public PtrList<FCall>
{
public:
	TypePreamble() {}
	virtual ~TypePreamble() {}

	void print(std::ostream& out) const { out << "PREAMBLE"; }
};

class TypeBlock : public TypeStmt, public PtrList<TypeStmt>
{
public:
	TypeBlock() {}
	~TypeBlock() 
	{
		for (iterator it = begin(); it != end(); it++)
			delete (*it);
	}

	void print(std::ostream& out) const 
	{
		out << "BLOCK:" << std::endl; 
		for (const_iterator it = begin(); it != end(); it++) {
			(*it)->print(out);
			out << std::endl;
		}
		out << "ENDBLOCK";
	}

	std::vector<TypeBB> getBB() const
	{
		std::vector<TypeBB>	ret;
		TypeBB			cur_bb;

		for (const_iterator it = begin(); it != end(); it++) {
			TypeStmt*	s;

			if (dynamic_cast<TypeDecl*>(s) != NULL) {
				cur_bb.add(s);
			} else {
				ret.push_back(cur_bb);
				cur_bb.clear();
			}
		}

		if (cur_bb.size() != 0) {
			ret.push_back(cur_bb);
		}

		return ret;
	}

	Expr* getBytes(uint32_t bmp) const
	{
		
	}

	Expr* getBits(uint32_t bmp) const
	{
		
	}
};

class TypeUnion : public TypeStmt
{
public:
	TypeUnion(const TypeBlock* tb, const Id* name)
	{
	}
	TypeUnion(const TypeBlock* tb, const IdArray* array)
	{
	}

	void print(std::ostream& out) const { out << "UNION"; }
};


class TypeFunc : public TypeStmt
{
public:
	TypeFunc(const FCall* fcall) {} 
	virtual ~TypeFunc() {}
	void print(std::ostream& out) const { out << "TYPEFUNC"; }
};


class Type : public GlobalStmt 
{
public:

	Type(	Id* in_name, ArgsList* in_args, TypePreamble* in_preamble, 
		TypeBlock* in_block)
	: name(in_name),
	  args(in_args),
	  preamble(in_preamble),
	  block(in_block)
	{
		assert (in_name != NULL);
		assert (in_block != NULL);
	}

	void print(std::ostream& out) const 
	{
		Expr	*bits, *bytes;

		bits = getBits(0);
		bytes = getBytes(0);

		out << "Type (";
		name->print(out);
		out << ") "; 
		bits->print(out);
		out << "b / ";
		bytes->print(out);

		block->print(out);
		out << std::endl;
		out << "End Type";

		delete bits;
		delete bytes;
	}

	virtual ~Type(void)
	{
		delete name;
		if (args != NULL) delete args;
		if (preamble != NULL) delete preamble;
		delete block;
	}


	Expr* getBytes(uint32_t bmp) const
	{
		return block->getBytes(bmp);
	}

	Expr* getBits(uint32_t bmp) const
	{
		return block->getBits(bmp);
	}

private:
	Id		*name;
	ArgsList	*args;
	TypePreamble	*preamble;
	TypeBlock	*block;
	
};


std::ostream& operator<<(std::ostream& in, const Type& t);

#endif
