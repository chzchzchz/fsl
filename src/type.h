#ifndef TYPE_H
#define TYPE_H

#include <iostream>
#include <list>
#include <map>
#include <assert.h>
#include <stdint.h>

#include "AST.h"
#include "collection.h"
#include "expr.h"

typedef std::map<std::string, class PhysicalType*> ptype_map;

class TypeStmt
{
public:
	virtual ~TypeStmt() {}
	virtual void print(std::ostream& out) const = 0;
	unsigned int getLineNo() const { return lineno; }
	void setLineNo(unsigned int l) { lineno = l; }

	virtual PhysicalType* resolve(const ptype_map& tm) const = 0;
protected:
	TypeStmt() {}
private:
	unsigned int lineno;
};

#if 0
class TypeBB : public PtrList<TypeStmt>
{
public:
	TypeBB() {}
	~TypeBB() {}
private:
};
#endif

static inline unsigned int tstmt_count_conds(const TypeStmt* t);

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

	virtual ~TypeDecl();

	void print(std::ostream& out) const;
	
	virtual PhysicalType* resolve(const ptype_map& tm) const;

	const std::string getName() const 
	{
		if (name != NULL)
			return name->getName();
		return array->getName();
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

	void print(std::ostream& out) const;

	virtual PhysicalType* resolve(const ptype_map& tm) const;

	const std::string getName() const 
	{
		if (name != NULL)
			return name->getName();
		return array->getName();
	}

private:
	Id*		name;
	IdArray*	array;
	FCall*		type;
};


class TypeCond: public TypeStmt
{
public:
	TypeCond(CondExpr* ce, TypeStmt* taken, TypeStmt* nottaken = NULL) 
	: cond(ce), is_true(taken), is_false(nottaken)
	{
		assert (cond != NULL);
		assert (is_true != NULL);
	}

	virtual ~TypeCond(void)
	{
		delete cond;
		if (is_false != NULL) delete is_false;
		delete is_true;
	}

	void print(std::ostream& out) const { out << "TYPECOND"; }

	const TypeStmt* getTrueStmt() const { return is_true; }
	const TypeStmt* getFalseStmt() const { return is_false; }

	virtual PhysicalType* resolve(const ptype_map& tm) const;
private:
	CondExpr	*cond;
	TypeStmt	*is_true;
	TypeStmt	*is_false;
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
	virtual ~TypeBlock() {}

	void print(std::ostream& out) const;
#if 0
	std::vector<TypeBB> getBB() const;
#endif

	/* get number of conditions */
	unsigned int getNumConds(void) const
	{
		unsigned int	ret;

		ret = 0;
		for (const_iterator it = begin(); it != end(); it++) {
			ret += tstmt_count_conds(*it);
		}

		return ret;
	}

	virtual PhysicalType* resolve(const ptype_map& tm) const;
};

class TypeUnion : public TypeStmt
{
public:
	TypeUnion(TypeBlock* in_block, Id* in_name)
	: block(in_block),
	  name(in_name),
	  array(NULL)
	{
		assert (block != NULL);
		assert (name != NULL);
	}

	TypeUnion(TypeBlock* in_block, IdArray* in_array)
	: block(in_block),
	  name(NULL),
	  array(in_array)
	{
		assert (block != NULL);
		assert (array != NULL);
	}

	virtual ~TypeUnion()
	{
		if (name != NULL) delete name;
		if (array != NULL) delete array;
		delete block;
	}

	void print(std::ostream& out) const { 
		out << "UNION: "; 
		block->print(out);
	}

	virtual PhysicalType* resolve(const ptype_map& tm) const;
private:
	TypeBlock	*block;
	Id		*name;
	IdArray		*array;
};


class TypeFunc : public TypeStmt
{
public:
	TypeFunc(FCall* in_fcall) 
		: fcall(in_fcall)
	{
		assert (fcall != NULL);
	} 

	virtual ~TypeFunc() 
	{
		delete fcall;
	}

	void print(std::ostream& out) const { out << "TYPEFUNC"; }

	const std::string& getName(void) const { return fcall->getName(); }

	virtual PhysicalType* resolve(const ptype_map& tm) const;
private:
	FCall*	fcall;
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

	void print(std::ostream& out) const;

	virtual ~Type(void);

	const std::string& getName(void) const { return name->getName(); }

	PhysicalType* resolve(const ptype_map& tm) const;

	const ArgsList* getArgs(void) const { return args; }

	class SymbolTable* getSyms(const ptype_map& tm) const;

private:
	Id		*name;
	ArgsList	*args;
	TypePreamble	*preamble;
	TypeBlock	*block;
	
};

class TypeCtx 
{
public:
	TypeCtx(Expr* in_offset) : offset(in_offset)
	{
		assert (offset != NULL);
	}

	virtual ~TypeCtx() { delete offset; }


private:
	Expr	*offset;
};


std::ostream& operator<<(std::ostream& in, const Type& t);

static unsigned int tstmt_count_conds(const TypeStmt* t)
{
	const TypeBlock	*b;
	const TypeCond	*c;

	if (t == NULL) return 0;

	b = dynamic_cast<const TypeBlock*>(t);
	if (b != NULL)
		return b->getNumConds();

	c = dynamic_cast<const TypeCond*>(t);
	if (c != NULL) {
		return	1 + 
			tstmt_count_conds(c->getTrueStmt()) + 
			tstmt_count_conds(c->getFalseStmt());
	}


	return 0;
}

#endif
